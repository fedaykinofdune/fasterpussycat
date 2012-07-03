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


unsigned char detected_success(struct http_request *req, struct http_response *res, void *data){
  return DETECT_SUCCESS;
}


unsigned char next_rule(struct http_request *req, struct http_response *res, void *data){
  return DETECT_NEXT_RULE;
}



unsigned char detected_fail(struct http_request *req, struct http_response *res, void *data){
  return DETECT_FAIL;
}




/* Compares the checksums for two responses: */

int same_page(struct http_sig* sig1, struct http_sig* sig2) {
  u32 i, bucket_fail = 0;
  s32 total_diff  = 0;
  u32 total_scale = 0;

  if (sig1->code != sig2->code) return 0;

  for (i=0;i<FP_SIZE;i++) {
    s32 diff = sig1->data[i] - sig2->data[i];
    u32 scale = sig1->data[i] + sig2->data[i];

    if (abs(diff) > 1 + (scale * FP_T_REL / 100) ||
        abs(diff) > FP_T_ABS)
      if (++bucket_fail > FP_B_FAIL) return 0;

    total_diff  += diff;
    total_scale += scale;

  }

  if (abs(total_diff) > 1 + (total_scale * FP_T_REL / 100))
    return 0;

  debug("same page");
  return 1;

}


struct match_rule *new_rule(struct match_rule **list){
  struct match_rule *rule;
  rule=calloc(sizeof(struct match_rule),1);
  rule->next=*list;
  
  rule->evaluate=next_rule;
  *list=rule;
  return rule;
}

int run_rules(struct match_rule *list, struct http_request *req, struct http_response *res){
  struct match_rule *rule;
  int rc=DETECT_NEXT_RULE;
  for(rule=list;rule!=NULL;rule=rule->next){
    if(rule_matches(rule,req,res)) rc=rule->evaluate(req,res,rule->data);
    if(rc!=DETECT_NEXT_RULE) return rc;
  }
  return DETECT_NEXT_RULE;
}


int not_head_method(struct http_request *req){
  return strcmp((char *) req->method, "HEAD");
}

int rule_matches(struct match_rule *rule, struct http_request *req, struct http_response *res){
  if(rule->method!=DETECT_ANY && strcmp((char *) req->method,(char *) rule->method)) return 0;
  if(rule->code!=DETECT_ANY && res && res->code!=rule->code) return 0;
  if(rule->size!=DETECT_ANY 
    && res
    && GET_HDR((unsigned char *) "content-length", &res->hdr) 
    && atoi((char *) GET_HDR((unsigned char *) "content-length", &res->hdr))!=rule->size) return 0;
  if(rule->test_flags!=DETECT_ANY && req->test && !(req->test->flags & rule->test_flags)) return 0;
  if(rule->hash!=DETECT_ANY 
    && res
    && not_head_method(req) 
    && res->md5_digest
    && memcmp(res->md5_digest,rule->hash,MD5_DIGEST_LENGTH)) return 0; 

  
/*  if(rule->hash!=DETECT_ANY 
    && res
    && not_head_method(req) 
    && res->md5_digest
    && memcmp(res->md5_digest,rule->hash,MD5_DIGEST_LENGTH)) return 0; */

  if(rule->pattern!=DETECT_ANY && regexec(rule->pattern, (char *) serialize_path(req,0,0), 0, NULL, 0)) return 0;
  if(rule->sig!=DETECT_ANY && res && not_head_method(req) && !same_page(rule->sig,&res->sig)) return 0;
  return 1;
}
