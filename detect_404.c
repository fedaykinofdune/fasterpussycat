#include <sys/types.h>
#include <regex.h>
#include <openssl/md5.h>
#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "util.h"
#include "match_rule.h"
#include "detect_404.h"





unsigned char keep_dir_count(struct http_request *req, struct http_response *res, void *data){
  struct detect_404_info *info=(struct detect_404_info *) data;
  if(info->dir_blacklist || !req->test || !(req->test->flags & F_DIRECTORY)) return DETECT_NEXT_RULE;
  if(info->last_dir_code==res->code && res->code==403){
    info->in_a_row_dir++;
    if(info->in_a_row_dir>10){
      struct match_rule *r;
      warn("Over 10 dir %ds in a row, marking directories with this code as unknown for %s",res->code,req->t->host);
      r=new_404_rule(info,&info->rules_general);
      r->test_flags=F_DIRECTORY,
      r->code=res->code;
      r->evaluate=detected_unknown;
      info->dir_blacklist=1;
      return DETECT_UNKNOWN;
    }
  }
  else{
    info->in_a_row_dir=0;
    info->last_dir_code=res->code;
  }
  return DETECT_NEXT_RULE;

}



unsigned char keep_hash_count(struct http_request *req, struct http_response *res, void *data){
  struct detect_404_info *info=(struct detect_404_info *) data;
  struct hash_count *h_count;

  if(!res->md5_digest) return DETECT_NEXT_RULE;

  if(!GET_HDR((unsigned char *) "content-length", &res->hdr) 
    || atoi((char *) GET_HDR((unsigned char *) "content-length", &res->hdr))<20) return DETECT_NEXT_RULE;

  HASH_FIND(hh, info->hash_counts, res->md5_digest, MD5_DIGEST_LENGTH, h_count);
  if(!h_count){
    h_count=detect_404_alloc(info,sizeof(struct hash_count));
    memcpy(&h_count->hash, res->md5_digest, MD5_DIGEST_LENGTH);
    HASH_ADD(hh, info->hash_counts, hash, MD5_DIGEST_LENGTH, h_count);
  }
  h_count->count++;
  if(h_count->count>10){
    warn("Over 10 responses with same hash for %s, blacklisting hash", req->host);
    blacklist_hash(info, res->md5_digest);
    return DETECT_FAIL;
  }

  return DETECT_NEXT_RULE;

}


/* generate a random lower case string */

void gen_random(unsigned char *s, const int len) {
  int i;
  static const char alphanum[] =
    "abcdefghijklmnopqrstuvwxyz";

  for (i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
}

void add_default_rules(struct detect_404_info *info){
  struct match_rule *rule;

  /* base rule is to return 404 for everything */

  rule=new_404_rule(info,&info->rules_final);
  rule->evaluate=detected_fail;

  /* and success for 200 OK */

  rule=new_404_rule(info,&info->rules_final);
  rule->code=200;
  rule->evaluate=detected_success;

  /* and for 401 and 403 for directories */

  rule=new_404_rule(info,&info->rules_final);
  rule->code=401;
  rule->test_flags=F_DIRECTORY;
  rule->evaluate=detected_success;

  rule=new_404_rule(info,&info->rules_final);
  rule->code=403;
  rule->test_flags=F_DIRECTORY;
  rule->evaluate=detected_success;

  /* response code of 0 means something went wrong */

  rule=new_404_rule(info,&info->rules_final);
  rule->evaluate=detect_error;

  /* prevent too many of the same hash */

  rule=new_404_rule(info,&info->rules_final);
  rule->code=200;
  rule->evaluate=keep_hash_count;
  rule->data=info;

  /* prevent too many dir successs */

  rule=new_404_rule(info,&info->rules_final);
  rule->test_flags=F_DIRECTORY;
  rule->evaluate=keep_dir_count;
  rule->data=info;

}


unsigned char detect_error(struct http_request *req, struct http_response *res, void *data){
  if(res->code==0) return DETECT_UNKNOWN;
  return DETECT_NEXT_RULE;
}


char * recommend_method(struct detect_404_info *info, struct url_test *test){
  struct recommended_method *r;
  for(r=info->recommended_request_methods; r!=NULL; r=r->next){
    if((r->pattern && !regexec(r->pattern, test->url, 0, NULL, 0))
    || (r->flags && (r->flags & test->flags))) return (char *) r->method; 
  }
  return "HEAD";
}


struct match_rule *new_404_rule(struct detect_404_info *info, struct match_rule **list){
  struct match_rule *rule;
  struct detect_404_cleanup_info *new_cleanup=malloc(sizeof(struct detect_404_cleanup_info));
  rule=new_rule(list);
  new_cleanup->data=rule;
  new_cleanup->cleanup_func=free;
  new_cleanup->next=info->cleanup;
  info->cleanup=new_cleanup;
  return rule;
}


void *detect_404_alloc(struct detect_404_info *info, size_t size){
  void *mem=calloc(size,1);
  struct detect_404_cleanup_info *new=malloc(sizeof(struct detect_404_cleanup_info));
  new->data=mem;
  new->cleanup_func=free;
  new->next=info->cleanup;
  info->cleanup=new;
  return mem;
}
void detect_404_cleanup(struct detect_404_info *info){
   struct detect_404_cleanup_info *c=info->cleanup;
   struct detect_404_cleanup_info *next=info->cleanup;
   while(c){
      next=c->next;
      c->cleanup_func(c->data);
      free(c);
      c=next;
   }
}


int determine_404_method(struct detect_404_info *info, struct request_response *list){
  struct request_response *r;
  int use_404=1, use_sig=1;
  #ifdef USE_HASH_404
  int use_hash=1;
  unsigned char *last_hash;
  #endif
  struct http_sig *last_sig;
  
  debug("in determine 404 method");
  
  /* caught by current rules */

  if(!list) return USE_UNKNOWN;



  for(r=list;r!=NULL;r=r->next){
    if(r->res->code==0){
      return USE_UNKNOWN;
      break;
    }
  }


  for(r=list;r!=NULL;r=r->next){
    if(is_404(info,r->req,r->res)!=DETECT_FAIL){
      use_404=0;
      break;
    }
  }
  if(use_404) return USE_404;
  
  
  /* otherwise resort to more primitive methods */
  #ifdef USE_HASH_404
  last_hash=list->res->md5_digest;
  if(last_hash){
    for(r=list;r!=NULL;r=r->next){
      if(memcmp(r->res->md5_digest,last_hash,MD5_DIGEST_LENGTH)){
        use_hash=0;
        break;
      }
    }

    if(use_hash) return USE_HASH;
  }
  #endif
  last_sig=&list->res->sig;
  for(r=list;r!=NULL;r=r->next){
    if(!same_page(last_sig,&r->res->sig)){
      use_sig=0;
      break;
    }
  }

  if(use_sig) return USE_SIG;

  return USE_UNKNOWN;

}

void blacklist_hash(struct detect_404_info *info, unsigned char *hash){
  struct match_rule *rule=new_404_rule(info,&info->rules_general);
  rule->code=200;
  rule->hash=detect_404_alloc(info,MD5_DIGEST_LENGTH);
  rule->evaluate=detected_fail;
  memcpy(rule->hash,hash,MD5_DIGEST_LENGTH);
}



void blacklist_sig(struct detect_404_info *info, struct http_sig *sig){
  struct match_rule *rule=new_404_rule(info,&info->rules_general);
  rule->code=200;
  rule->sig=detect_404_alloc(info,sizeof(struct http_sig));
  rule->evaluate=detected_fail;
  memcpy(rule->sig,sig,sizeof(struct http_sig));
}

void enqueue_other_probes(struct target *t){
  char *check_ext[] = CHECK_EXT;
  struct probe *probe;
  struct http_request *req;
  int i,j;
  char *path=malloc(20);

  /* enqueue extention checks */

  for(i=0;i<CHECK_EXT_LEN;i++){
    probe=malloc(sizeof(struct probe));
    probe->type=PROBE_EXT;
    probe->responses=NULL;
    probe->data=check_ext[i];
    probe->count=3;
    t->detect_404->probes++;
    for(j=0;j<3;j++){
      gen_random((unsigned char *) path,8);
      path[0]='/';
      strcat(path,".");
      strcat(path, check_ext[i]);
      req=new_request_with_method_and_path(t,(unsigned char *) "GET", (unsigned char *) path);
      req->data=probe;
      req->callback=process_probe;
      async_request(req);
    }
  }

  /* enqueue directory checks */

  probe=malloc(sizeof(struct probe));
  probe->type=PROBE_DIR;
  probe->responses=NULL;
  probe->data=NULL;
  probe->count=3;
  t->detect_404->probes++;
  for(j=0;j<3;j++){
    gen_random((unsigned char *) path,8);
    path[0]='/';
    strcat(path,"/");
    req=new_request_with_method_and_path(t,(unsigned char *) "GET", (unsigned char *) path);
    req->data=probe;
    req->callback=process_probe;
    async_request(req);
  }

  /* enqueue cgi checks */

  probe=malloc(sizeof(struct probe));
  probe->type=PROBE_CGI;
  probe->responses=NULL;
  probe->data=NULL;
  probe->count=3;
  t->detect_404->probes++;
  for(j=0;j<3;j++){
    strcpy(path,"/cgi-bin/");
    gen_random((unsigned char *) &path[9],7);
    req=new_request_with_method_and_path(t,(unsigned char *) "GET", (unsigned char *) path);
    req->data=probe;
    req->callback=process_probe;
    async_request(req);
  }

  free(path);

}

u8 process_probe(struct http_request *req,struct http_response *res){
  struct probe *probe=(struct probe *) req->data;
  struct request_response *response=calloc(sizeof(struct request_response),1);
  char *pattern=NULL;
  struct recommended_method *rec_method;
  regex_t *regex=NULL;
  int flags=0;
  int detect_method;
  char *http_method="HEAD";
  char *type_str=NULL;
  debug("process probe");
  response->req=req;
  response->res=res;
  response->next=probe->responses;
  probe->responses=response;
  probe->count--;

  if(probe->count){
    return 1;
  }
  req->t->detect_404->probes--;
  debug("determining method");
  detect_method=determine_404_method(req->t->detect_404,probe->responses);

  switch(detect_method){
    case USE_404: 
      /* all g in the h */
      
      if(probe->type==PROBE_CGI) debug("404 cgi");
      type_str="404";
      break;
    case USE_HASH:
      blacklist_hash(req->t->detect_404,res->md5_digest);
      debug("blacklisting hash");
      type_str="hash";
      break;
    case USE_SIG:
      blacklist_sig(req->t->detect_404,&res->sig);
      debug("blacklisting sig");
      if(probe->type==PROBE_CGI) debug("blacklist cgi sig");
      type_str="sig";
      break;
    case USE_UNKNOWN:
      debug("use unknown");
  }
  if(detect_method!=USE_404){
    http_method="GET";
    switch(probe->type){
      case PROBE_GENERAL:
        pattern=strdup(".*");
        if(detect_method==USE_UNKNOWN){
          warn("404 detect failed on %s",req->t->host);
          return 1;
        }
        break;
      case PROBE_EXT:
        pattern=malloc(strlen(probe->data)+11);
        pattern[0]=0;
        strcat(pattern,".*\\.");
        strcat(pattern,probe->data);
        strcat(pattern,"(\\?|$)");
        break;

      case PROBE_CGI:
        pattern=strdup("^/cgi-bin/.*");
        break;
      case PROBE_DIR:
        flags=F_DIRECTORY;
        break;
    }

    if(pattern){
      regex=malloc(sizeof(regex_t));
      if(regcomp(regex, pattern, REG_EXTENDED | REG_NOSUB) ) fatal("Could not compile regex %s",pattern);
    }
    if(detect_method==USE_UNKNOWN){ 
      http_method=RECOMMEND_SKIP;
    }
    rec_method=detect_404_alloc(req->t->detect_404,sizeof(struct recommended_method));
    rec_method->pattern=regex;
    rec_method->method=(unsigned char *) http_method;
    rec_method->flags=flags;
    rec_method->next=req->t->detect_404->recommended_request_methods;
    req->t->detect_404->recommended_request_methods=rec_method;
    debug("added request recommendations pattern: %s flags: %d method %s", pattern, flags, http_method); 
    free(pattern);
  } /* if(detect_method!=USE_404) */
  if(probe->type==PROBE_GENERAL) info("404 detection method for %s: %s", req->t->host, type_str);

  if(probe->type==PROBE_GENERAL) enqueue_other_probes(req->t);
  else if(!req->t->detect_404->probes) enqueue_tests(req->t);
  return 1; 
}

void launch_404_probes(struct target *t){
  int i;
  struct http_request *req;
  char *path=malloc(9);
  struct probe *probe=malloc(sizeof(struct probe));
  info("launching probes for %s",t->host);
  probe->count=3;
  probe->type=PROBE_GENERAL;
  probe->data=NULL;
  probe->responses=NULL;
  t->detect_404->probes++;
  /* first launch 3 general probes */
  for(i=0;i<3;i++){
    gen_random((unsigned char *) path,8);
    path[0]='/';
    req=new_request_with_method_and_path(t,(unsigned char*) "GET",(unsigned char*) path);
    req->callback=process_probe;
    req->data=probe;
    async_request(req);
  }
}

void destroy_detect_404_info(struct detect_404_info *info){
  detect_404_cleanup(info);
  free(info);
}

int is_404(struct detect_404_info *info, struct http_request *req, struct http_response *res){
  int rc=DETECT_NEXT_RULE;
  if((rc=run_rules(info->rules_preprocess,req,res))!=DETECT_NEXT_RULE) return rc;
  if((rc=run_rules(info->rules_general   ,req,res))!=DETECT_NEXT_RULE) return rc;
  if((rc=run_rules(info->rules_final     ,req,res))!=DETECT_NEXT_RULE) return rc;
  return DETECT_UNKNOWN;
}


