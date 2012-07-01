#include <sys/types.h>
#include <regex.h>
#include "match_rule.h"
#include "detect_404.h"


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
  struct *match_rule *rule;

  /* base rule is to return 404 for everything */

  rule=new_404_rule(info,&info->final);
  rule->evaluate=detected_404;

  /* and success for 200 OK */

  rule=new_404_rule(info,&info->final);
  rule->code=200;
  rule->evaluate=detected_success;

  /* and for 401 and 403 for directories */

  rule=new_404_rule(info,&info->final);
  rule->code=401;
  rule->test_flags=F_DIRECTORY;
  rule->evaluate=detected_success;

  rule=new_404_rule(info,&info->final);
  rule->code=403;
  rule->test_flags=F_DIRECTORY;
  rule->evaluate=detected_success;

  /* response code of 0 means something went wrong */

  rule=new_404_rule(info,&info->final);
  rule->evaluate=detect_error;

}



unsigned char detect_error(struct http_request *req, struct http_response *res, void *data){
  if(res->code==0) return DETECT_UNKNOWN;
  return DETECT_NEXT_RULE;
}





struct match_rule *new_404_rule(struct detect_404_info *info, struct match_rule **list){
  struct match_rule *rule;
  struct detect_404_cleanup *new_cleanup=malloc(sizeof(struct detect_404_cleanup));
  rule=new_match_rule(list);
  new->data=rule;
  new->cleanup_func=free;
  new->next=info->cleanup;
  return rule;
}


void *detect_404_alloc(struct detect_404_info *info, size_t size){
  void *mem=calloc(size,1);
  struct detect_404_cleanup *new=malloc(sizeof(struct detect_404_cleanup));
  new->data=mem;
  new->cleanup_func=free;
  new->next=info->cleanup;
  info->cleanup=new;

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


void determine_404_method(struct detect_404_info *info, struct request_response *list){
  struct request_response *r;
  int use_404=1, use_hash=1, use_sig=1;
  unsigned char *last_hash;
  http_sig *sig;
  /* caught by current rules */

  if(!list) return USE_UNKNOWN;

  while(r=list;r!=NULL;r=r->next){
    if(is_404(info,list->req,list->res)!=DETECT_FAIL){
      use_404=0;
      break;
    }
  }
  if(use_404) return USE_404;
  
  
  /* otherwise resort to more primitive methods */
  
  last_hash=list->rep->md5hash;
  while(r=list;r!=NULL;r=r->next){
    if(memcmp(r->rep->md5_hash,last_hash,MD5_DIGEST_LENGTH)){
      use_hash=0;
      break;
    }
  }

  if(use_hash) return USE_HASH;

  last_sig=list->rep->sig;
  while(r=list;r!=NULL;r=r->next){
    if(!same_page(last_sig,r->rep->sig)){
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
  memcpy(rule->hash,hash,MD5_DIGEST_LENGTH);
}



void blacklist_sig(struct detect_404_info *info, struct http_sig *sig){
  struct match_rule *rule=new_404_rule(info,&info->rules_general);
  rule->code=200;
  rule->sig=detect_404_alloc(info,sizeof(struct http_sig));
  memcpy(rule->sig,sig,sizeof(struct http_sig));
}

void enqueue_other_probes(struct *target *t){
  char *check_ext[] = CHECK_EXT;
  struct probe *probe;
  struct http_request *req;
  int i,j;
  char *path=malloc(15);

  /* enqueue extention checks */

  for(i=0;i<CHECK_EXT_LEN;i++){
    probe=malloc(sizeof(struct probe));
    probe->type=PROBE_EXT;
    probe->responses=NULL;
    probe->data=check_ext[i];
    probe->count=3;
    t->detect_404->probes++;
    for(j=0;j<3;j++){
      gen_random(path,8);
      path[0]='/';
      strcat(path,".");
      strcat(check_ext[i]);
      req=new_request_with_method_and_path(t,"GET", path);
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
    gen_random(path,8);
    path[0]='/';
    strcat(path,"/");
    req=new_request_with_method_and_path(t,"GET", path);
    req->data=probe;
    req->callback=process_probe;
    async_request(req);
  }

}

u8 process_probe(struct http_request *req,struct http_response *res){
  struct probe *probe=(struct *probe) req->data;
  struct request_response *response=calloc(sizeof(struct request_response),1);
  char *pattern=NULL;
  unsigned char *hash=NULL;
  struct http_sig=NULL;
  struct match_rule *rule;
  regex_t *regex;
  int flags=0;
  int detect_method;
  char *recommended_request_method="GET";
  response->req=req;
  response->res=res;
  response->next=probe->responses;
  probe->responses=response;
  probe->count--;

  if(probe->count){
    return 1;
  }
  req->t->detect_404->probes--;
  detect_method=determine_404_method(req->t->detect_404,probe->response);

  if(detect_method==USE_404){ /* all g in the h */
    if(probe->type==PROBE_GENERAL) enqueue_other_probes(req->t);
    return 1;
  }

  switch(detect_method){
    case USE_404: 
      /* all g in the h */
      break;
    case USE_HASH:
      blacklist_hash(req->t->detect_404,res->md5_digest);
      break;
    case USE_SIG;
      blacklist_sig(req->t->detect_404,res->sig);
      break;
  }
  if(detect_method!=USE_404){
    switch(probe->type);
      case PROBE_GENERAL:
        if(detect_method==USE_UNKNOWN){
          warn("404 detect failed on %s",req->t->host);
          return 1;
        }
        break;
      case PROBE_EXT:
        pattern=malloc(strlen(probe->data)+11);
        pattern[0]=0'
        strcat(reg,".*\\.");
        strcat(reg,data);
        strcat(reg,"(\\?|$)");
        break;
    case PROBE_DIR:
        flags=F_DIRECTORY;
        break;
    }
  }
  if(pattern){
    regex=malloc(sizeof(regex_t));
    if(regcomp(&regex, pattern, 0) ) fatal("Could not compile regex %s",pattern);
  }
  if(detect_method==USE_UNKNOWN){ 
    recommended_request_method=NULL;
  }
  rule=new_rule(req->t->detect_404->recommended_request_methods);
  rule->pattern=regex;
  rule->flags=flags;
  
  if(probe->type==PROBE_GENERAL) enqueue_other_probes(req->t);
  else if(!req->t->detect_404->probes) enqueue_tests(req->t)
    
}

void launch_404_probes(struct target *t){
  int i;
  struct http_request *req;
  char *path=malloc(9);
  struct probe *probe=malloc(sizeof(struct probe));
  probe->count=3;
  probe->type=PROBE_GENERAL;
  probe->data=NULL;
  probe->response=NULL;
  t->detect_404->probes++;
  /* first launch 3 general probes */
  for(i=0;i<3;i++){
    gen_random(path,8);
    path[0]='/';
    req=new_request_with_method_and_path(t,"GET",path);
    req->callback=process_probe;
    req->data=probe;
    async_request(req);
  }
}

void destroy_detect_404_info(struct detect_404_info *info){
  detect_404_cleanup(info);
  free(info);
}

void is_404(struct detect_404_info *info, struct http_request *req, struct http_response *rep){
  int rc=DETECT_NEXT_RULE;
  if((rc=run_rules(info->rules_preprocess,req,rep))!=DETECT_NEXT_RULE) return rc;
  if((rc=run_rules(info->rules_general   ,req,rep))!=DETECT_NEXT_RULE) return rc;
  if((rc=run_rules(info->rules_final     ,req,rep))!=DETECT_NEXT_RULE) return rc;
  return DETECT_UNKNOWN;
}



