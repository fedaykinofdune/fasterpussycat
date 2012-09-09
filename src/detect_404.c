#include <sys/types.h>
#include <regex.h>
#include <openssl/md5.h>
#include <magic.h>
#include "engine.h"
#include "util.h"
#include "detect_404.h"



int blacklist_success=1;
int skip_other_probes=0;
int skip_sig=0;
magic_t magic=NULL;


unsigned char keep_dir_count(struct http_request *req, struct http_response *res, void *data){
  struct detect_404_info *info=(struct detect_404_info *) data;
  if(info->dir_blacklist || !req->test || !(req->test->flags & F_DIRECTORY)) return DETECT_NEXT_RULE;
  if(info->last_dir_code==res->code && (res->code==403 || res->code==401 || res->code==200)){
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



  /* and success for 500 */

  rule=new_404_rule(info,&info->rules_final);
  rule->code=500;
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

  rule=new_404_rule(info,&info->rules_final);
  rule->code=500;
  rule->evaluate=keep_hash_count;
  rule->data=info;

  /* prevent too many dir successs */

  rule=new_404_rule(info,&info->rules_final);
  rule->test_flags=F_DIRECTORY;
  rule->evaluate=keep_dir_count;
  rule->data=info;

  create_magic_rules(info);

}


unsigned char enforce_magic_rule(struct http_request *req, struct http_response *res, void *data){
  const char *mime=magic_buffer(magic, res->payload,res->pay_len);
  if(!mime || strcmp(mime,(char *) data)){
    return DETECT_FAIL;
  }
  return DETECT_SUCCESS;
}

void create_magic_rules(struct detect_404_info *info){
  if(magic==NULL){
    magic=magic_open(MAGIC_MIME_TYPE);
    if(magic==NULL){
      fatal("couldn't open magic database");
    }
    magic_load(magic,NULL);
  }
  create_magic_rule(info, "sql",  "text/plain");
  create_magic_rule(info, "tar",  "application/x-tar");
  create_magic_rule(info, "mdb",  "application/x-msaccess");
  create_magic_rule(info, "gz",   "application/x-gzip");
  create_magic_rule(info, "tgz",  "application/x-gzip");
  create_magic_rule(info, "zip",  "application/zip");
  create_magic_rule(info, "bz2",  "application/x-bzip2");
}

void create_magic_rule(struct detect_404_info *info, char *ext, char *mime_type){
  struct match_rule *rule=new_404_rule(info,&info->rules_preprocess);
  char *pattern=ck_alloc(strlen(ext)+6);
  regex_t *regex=detect_404_alloc(info,sizeof(regex_t));
  pattern[0]=0;
  strcat(pattern,"\\.");
  strcat(pattern,ext);
  strcat(pattern,"$");
  
  if(regcomp(regex, pattern, REG_EXTENDED | REG_NOSUB) ) fatal("Could not compile regex '%s'",pattern);
  rule->code=200;
  rule->method=(unsigned char *) "GET";
  rule->pattern=regex;
  rule->data=mime_type;
  rule->evaluate=enforce_magic_rule;
  ck_free(pattern);
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
  struct detect_404_cleanup_info *new_cleanup=ck_alloc(sizeof(struct detect_404_cleanup_info));
  rule=new_rule(list);
  new_cleanup->data=rule;
  new_cleanup->cleanup_func=free;
  new_cleanup->next=info->cleanup;
  info->cleanup=new_cleanup;
  return rule;
}


void *detect_404_alloc(struct detect_404_info *info, size_t size){
  void *mem=ck_alloc(size);
  struct detect_404_cleanup_info *new=ck_alloc(sizeof(struct detect_404_cleanup_info));
  new->data=mem;
  new->cleanup_func=ck_free;
  new->next=info->cleanup;
  info->cleanup=new;
  return mem;
}

void req_clean(void *m){
  destroy_request((struct http_request *) m);
}



void res_clean(void *m){
  destroy_response((struct http_response *) m);
}

void schedule_req_res_cleanup(struct detect_404_info *info, struct http_request *req, struct http_response *res){
  struct detect_404_cleanup_info *new;
  new=ck_alloc(sizeof(struct detect_404_cleanup_info));
  new->data=req;
  new->cleanup_func=req_clean;
  new->next=info->cleanup;
  info->cleanup=new;

  new=ck_alloc(sizeof(struct detect_404_cleanup_info));
  new->data=res;
  new->cleanup_func=res_clean;
  new->next=info->cleanup;
  info->cleanup=new;
}


void detect_404_cleanup(struct detect_404_info *info){
   struct detect_404_cleanup_info *c=info->cleanup;
   struct detect_404_cleanup_info *next=info->cleanup;
   while(c){
      next=c->next;
      c->cleanup_func(c->data);
      ck_free(c);
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
  
  rule=new_404_rule(info,&info->rules_general);
  rule->code=500;
  rule->hash=detect_404_alloc(info,MD5_DIGEST_LENGTH);
  rule->evaluate=detected_fail;
  memcpy(rule->hash,hash,MD5_DIGEST_LENGTH);

}



void blacklist_sig(struct detect_404_info *info, struct http_sig *sig){
  struct match_rule *rule=new_404_rule(info,&info->rules_general);
  rule->code=200;
  rule->sig=detect_404_alloc(info,sizeof(struct http_sig));
  rule->mime_type="text/html";
  rule->evaluate=detected_fail;
  memcpy(rule->sig,sig,sizeof(struct http_sig));
  rule=new_404_rule(info,&info->rules_general);
  rule->code=500;
  rule->sig=detect_404_alloc(info,sizeof(struct http_sig));
  rule->mime_type="text/html";
  rule->evaluate=detected_fail;
  memcpy(rule->sig,sig,sizeof(struct http_sig));
}

void enqueue_other_probes(struct target *t){
  char *check_ext[] = CHECK_EXT;
  struct probe *probe;
  struct http_request *req;
  int i,j;
  char *path=ck_alloc(50);

  /* enqueue extention checks */

  for(i=0;i<CHECK_EXT_LEN;i++){
    probe=ck_alloc(sizeof(struct probe));
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

  probe=ck_alloc(sizeof(struct probe));
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

  ck_free(path);

}

u8 process_probe(struct http_request *req,struct http_response *res){
  struct probe *probe=(struct probe *) req->data;
  struct request_response *response=detect_404_alloc(req->t->detect_404,sizeof(struct request_response));
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
  schedule_req_res_cleanup(req->t->detect_404, req, res);
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
        pattern=(char *) ck_strdup((u8 *) ".*");
        if(detect_method==USE_UNKNOWN){
          warn("404 detect failed on %s",req->t->host);
          return 1;
        }
        break;
      case PROBE_EXT:
        pattern=(char *) ck_alloc(strlen(probe->data)+11);
        strcat(pattern,".*\\.");
        strcat(pattern,probe->data);
        strcat(pattern,"(\\?|$)");
        break;

      case PROBE_CGI:
        pattern=(char *) ck_strdup((u8*) "^/cgi-bin/.*");
        break;
      case PROBE_DIR:
        flags=F_DIRECTORY;
        break;
    }

    if(pattern){
      regex=ck_alloc(sizeof(regex_t));
      if(regcomp(regex, pattern, REG_EXTENDED | REG_NOSUB) ) fatal("Could not compile regex %s",pattern);
      
      ck_free(pattern);
    }
    if(detect_method==USE_UNKNOWN || (detect_method!=USE_404 && skip_sig)){ 
      http_method=RECOMMEND_SKIP;
    }
    rec_method=detect_404_alloc(req->t->detect_404,sizeof(struct recommended_method));
    rec_method->pattern=regex;
    rec_method->method=(unsigned char *) http_method;
    rec_method->flags=flags;
    rec_method->next=req->t->detect_404->recommended_request_methods;
    req->t->detect_404->recommended_request_methods=rec_method;
    debug("added request recommendations pattern: %s flags: %d method %s", pattern, flags, http_method); 
  } /* if(detect_method!=USE_404) */
  if(probe->type==PROBE_GENERAL) info("404 detection method for %s: %s", req->t->host, type_str);
  if(probe->type==PROBE_GENERAL && !skip_other_probes) enqueue_other_probes(req->t);
  else if(!req->t->detect_404->probes) req->t->after_probes(req->t);
  return 1; 
}

void launch_404_probes(struct target *t){
  int i;
  struct http_request *req;
  char *path=malloc(9);
  struct probe *probe=detect_404_alloc(t->detect_404, sizeof(struct probe));
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
  free(path);
}

void destroy_detect_404_info(struct detect_404_info *info){
  detect_404_cleanup(info);
  ck_free(info);
}

int is_404(struct detect_404_info *info, struct http_request *req, struct http_response *res){
  int rc=DETECT_NEXT_RULE;
  if(rc==DETECT_NEXT_RULE) rc=run_rules(info->rules_preprocess,req,res);
  if(rc==DETECT_NEXT_RULE) rc=run_rules(info->rules_general,req,res);
  if(rc==DETECT_NEXT_RULE) rc=run_rules(info->rules_final,req,res);
  if((rc==DETECT_FAIL || rc==DETECT_UNKNOWN) && req->test && (req->test->flags & F_DIRECTORY)){
    info->in_a_row_dir=0;
    info->last_dir_code=404;
  }
   
  if(rc==DETECT_SUCCESS && blacklist_success && res->pay_len>50 && not_head_method(req)) blacklist_sig(info,&res->sig);
  if(rc==DETECT_SUCCESS && !not_head_method(req) && (res->code==200 || res->code==500) && req->test && req->t){
    /* reschedule with get */
    
    struct http_request *req2=req_copy(req,1);
    req2->callback=process_test_result;
    req2->test=req->test;
    req2->t=req->t;
    req2->soon=1;
    req2->score=999;
    ck_free(req2->method);
    req2->method=ck_strdup((unsigned char *) "GET");
    if(req->pointer){
      req2->pointer=req->pointer;
      req2->pointer->req=req2;
    }
    async_request(req2);
    rc=DETECT_UNKNOWN;
  }
  if(rc==DETECT_SUCCESS) info->success++;
  if(rc!=DETECT_UNKNOWN) info->count++;
  if(info->count>100){
 
    if(((double) info->success/(double) info->count)>0.3 && !info->removed){
      warn("30%% success rate over 100 queries, removing %s",req->host);
      info->removed=1;
      remove_host_from_queue(req->t->full_host);
    }
  }
 if(info->count>300 && train){
 
    if(((double) info->success/(double) info->count)>0.05 && !info->removed){
      warn("5%% success rate over 300 queries, removing %s",req->host);
      info->removed=1;
      remove_host_from_queue(req->t->full_host);
    }
  }
  
  return rc;
}



