#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include "db.h"
#include "match_rule.h"
#include "post.h"


static struct match_rule *post_rules=NULL;

struct annotation *annotate(struct http_response *res, char *key, char *value){
  struct annotation *a=calloc(sizeof(struct annotation),1);
  a->next=res->annotations;
  res->annotations=a;
  a->key=key;
  a->value=value;
  return a;
}



unsigned char php_error(struct http_request *req, struct http_response *res, void *data){
  static regex_t *regex=NULL;
  if(regex==NULL){
    regex=ck_alloc(sizeof(regex_t));
    regcomp(regex," <b>(Warning|Fatal error)</b>: .* in <b>([^<]+)</b> on line",REG_EXTENDED | REG_ICASE);
  }
  if(!regexec(regex, res->payload, 0, 0, 0)){
    annotate(res,"php-error",NULL);
  }
  return DETECT_NEXT_RULE;
}

unsigned char mysql_syntax_error(struct http_request *req, struct http_response *res, void *data){
  if(strstr(res->payload, "You have an error in your SQL syntax; check the manual that corresponds to your MySQL server version")){
    annotate(res,"mysql-syntax-error",NULL);
  }
  return DETECT_NEXT_RULE;
}



unsigned char phpinfo(struct http_request *req, struct http_response *res, void *data){
  if((strstr(res->payload, "alt=\"PHP Logo\"") && strstr(res->payload, "safe_mode_exec_dir")) || strstr(res->payload,"<title>phpinfo()</title>")){
    annotate(res,"phpinfo",NULL);
  }
  return DETECT_NEXT_RULE;
}

unsigned char postgres_error(struct http_request *req, struct http_response *res, void *data){
  if(strstr(res->payload, "PostgreSQL query failed")){
    annotate(res,"postgres-error",NULL);
  }
  return DETECT_NEXT_RULE;
}


unsigned char input_fields(struct http_request *req, struct http_response *res, void *data){
  static regex_t *regex=NULL;
  char *p;
  char input[200];
  int i, copy, seen_upload=0, seen_password=0;
  regmatch_t m[1];
  if(regex==NULL){
    regex=ck_alloc(sizeof(regex_t));
    regcomp(regex,"<input[^>]+",REG_EXTENDED | REG_ICASE);
  }
  p=(char *) res->payload;
  while(!regexec(regex, p, 1, m, 0)){
    copy=m[0].rm_eo-m[0].rm_so+1;
    if(copy>198) copy=198;
    memcpy(input,p+m[0].rm_so,copy);
    input[copy]=0;
    for(i=0;i<copy;i++){
      input[i]=tolower(input[i]);
    }
    if((strstr(input,"type='password'") ||
       strstr(input,"type=\"password\"") ||
       strstr(input,"type=password")) &&
       !seen_password){
      seen_password=1;
      annotate(res,"password-field",NULL);
    }

    if((strstr(input,"type='file'") ||
       strstr(input,"type=\"file\"") ||
       strstr(input,"type=file")) &&
       !seen_upload){
      seen_upload=1;
      annotate(res,"file-upload",NULL);
    }
    p=p+m[0].rm_eo;

  }
  return DETECT_NEXT_RULE;
}


unsigned char server_path_disclosure(struct http_request *req, struct http_response *res, void *data){
  regex_t *unix_r;
  regex_t *win_r;
  regmatch_t m[2];
  char *unix_p;
  char *win_p;
  char *ret;
  char *new_path;

  char *new_newpath;
  int s=0;
  int i=0;
  char *path=(char *) serialize_path(req,0,0);
  static regex_t *path_r=NULL;
  if(!path_r){
    path_r=calloc(sizeof(regex_t),1);
    if(regcomp(path_r, "^(.*?)($|[#?])", REG_EXTENDED | REG_ICASE)) fatal("Could not compile regex");
  }
  if(regexec(path_r, path, 2, m, 0)) return DETECT_NEXT_RULE;
  s=m[1].rm_eo-m[1].rm_so;
  if(!s) return DETECT_NEXT_RULE;
  new_path=calloc(1,s+1);
  memcpy(new_path,path+m[1].rm_so,s);
  unix_p=calloc(1,strlen(new_path)+100);
  strcat(unix_p,"/(var|www|usr|tmp|virtual|etc|home|mnt|mount|root|proc)/[a-z0-9_/.-]+");
  strcat(unix_p, new_path+1);
  unix_r=calloc(sizeof(regex_t),1);
  if(regcomp(unix_r, unix_p, REG_EXTENDED | REG_ICASE)) {
    warn("Could not compile regex '%s'",unix_p);
    free(unix_p);
    free(unix_r);
    return DETECT_NEXT_RULE;
  }

  free(unix_p);

  if(!regexec(unix_r, (char *) res->payload, 1, m, 0)){
    s=m[0].rm_eo-m[0].rm_so;
    ret=calloc(s+1,1);
    memcpy(ret,res->payload+m[0].rm_so,s);
    if(strstr(ret,new_path)){
      *strstr(ret,new_path)='/';
      *strstr(ret,new_path+1)=0;
    }
    free(new_path);
    regfree(unix_r);
    annotate(res,"server-path", ret);
    return DETECT_NEXT_RULE;
  }

  win_r=calloc(sizeof(regex_t),1);
  
  new_newpath=calloc(strlen(new_path)*2,1);
  int c=0;
  for(i=0;i<strlen(new_path);i++){

    if(new_path[i]=='/' || new_path[i]=='\\'){
      new_newpath[c]='\\';
      new_newpath[c+1]='\\';
      c=c+2;
    }
    else{
      new_newpath[c]=new_path[i];
      c++;
    }
  }
  new_newpath[c]=0;
  win_p=calloc(strlen(new_newpath)+200,1);

  strcat(win_p, "[a-z]:\\\\(program files|windows|inetpub|php|document and settings|www|winnt|xampp|wamp|temp|websites|apache|apache2|site|sites|htdocs|web|http)\\[\\\\a-z 0-9_-.]+");

  strcat(win_p, new_newpath);
  if(regcomp(win_r, win_p, REG_EXTENDED | REG_ICASE)){
    warn("Could not compile regex %s",win_p);
    free(win_p);
    free(win_r);
    return DETECT_NEXT_RULE;
  }
  free(win_p);

  if(!regexec(win_r, (char *) res->payload, 1, m, 0)){
    regfree(win_r);
    s=m[0].rm_eo-m[0].rm_so;
    ret=calloc(s+1,1);
    
    memcpy(ret,res->payload+m[0].rm_so,s);
    
    if(strstr(ret,new_newpath)){
      *strstr(ret,new_newpath)='/';
      *strstr(ret,new_newpath+1)=0;
    }
    free(new_newpath);
    annotate(res,"server-path", ret);
    return DETECT_NEXT_RULE;
  }
  return DETECT_NEXT_RULE;
}

unsigned char realm(struct http_request *req, struct http_response *res, void *data){
  static regex_t *realm_regex=NULL;
  regmatch_t m[2];
  char *realm;
  int size;
  char *auth;
  if(!realm_regex){
    realm_regex=malloc(sizeof(regex_t));
    regcomp(realm_regex, "realm=\"(.*)\"", REG_EXTENDED | REG_ICASE);
  }
  auth=(char *) GET_HDR((unsigned char *) "www-authenticate",&res->hdr);
  if(!auth) return DETECT_NEXT_RULE;
  if(regexec(realm_regex, auth, 2, m, 0)) return DETECT_NEXT_RULE;
  size=m[1].rm_eo-m[1].rm_so;
  realm=ck_alloc(size+1);
  memcpy(realm,auth+m[1].rm_so,size);
  annotate(res,"realm",realm);
  return DETECT_NEXT_RULE;
}

unsigned char index_of(struct http_request *req, struct http_response *res, void *data){
  static char * text[]={"<title>Directory Listing For",
						            		"Directory Listing for",
							            	"Last modified</a>",	
						             		"<TITLE>Folder Listing",
							             	"<table summary=\"Directory Listing\"",
                            0
                            };

	static char * patterns[]={
                                "<A\\sHREF=\"[^\"]*\">\\[To\\sParent\\sDirectory]</A>",
								                "<body><h1>Directory\\sListing\\sFor\\s.*</h1>",
								                "<HTML><HEAD><TITLE>Directory:.*?</TITLE></HEAD><BODY>",
								                "<a href=\"\\?C=[NMSD];O=[AD]\">Name</a>",
								                "<title>Index\\sof\\s.*?</title>",
                                0
                                };

  int i=0;
  int regex_c=0;
  for(regex_c=0;patterns[regex_c];regex_c++); /* count */
  static regex_t ** regex=NULL; 
  if(!regex) regex=calloc(sizeof(regex_t *),regex_c+1);
  if(!regex[0]){
    for(i=0;i<regex_c;i++){
      regex[i]=calloc(sizeof(regex_t),1);
      if(regcomp(regex[i], patterns[i], REG_EXTENDED | REG_ICASE | REG_NOSUB)) fatal("Could not compile regex");
    }
  }
  for(i=0;text[i];i++){
    if(strstr((char *) res->payload, (char *) text[i])){
    goto success;
    }
  }
  for(i=0;regex[i];i++){
    if(!regexec(regex[i],(char *) res->payload,0,NULL,0)){
      
      goto success;
    }
  }
  return DETECT_NEXT_RULE;

success:

  annotate(res,"index-of", NULL);
  return DETECT_NEXT_RULE;
}


void run_post_rules(struct http_request *req, struct http_response *res){
  run_rules(post_rules,req,res);
}

void add_post_rules(){
  struct match_rule *rule;
  rule=new_rule(&post_rules);
  rule->test_flags=F_DIRECTORY;
  rule->code=200;
  rule->evaluate=index_of;

  rule=new_rule(&post_rules);
  rule->code=200;
  rule->evaluate=server_path_disclosure;

  rule=new_rule(&post_rules);
  rule->code=200;
  rule->evaluate=input_fields;

  rule=new_rule(&post_rules);
  rule->code=200;
  rule->evaluate=php_error;

  rule=new_rule(&post_rules);
  rule->code=200;
  rule->evaluate=mysql_syntax_error;

  rule=new_rule(&post_rules);
  rule->code=200;
  rule->evaluate=postgres_error;

  rule=new_rule(&post_rules);
  rule->code=200;
  rule->evaluate=phpinfo;

  rule=new_rule(&post_rules);
  rule->code=401;
  rule->evaluate=realm;

}


