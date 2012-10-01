#include "match_rule.h"

static char *to_compile=NULL;
extern struct ast_node *parsed_ast;
int globalReadOffset=0;
unsigned char detected_success(struct http_request *req, struct http_response *res, void *data){
  return DETECT_SUCCESS;
}



unsigned char detected_unknown(struct http_request *req, struct http_response *res, void *data){
  return DETECT_UNKNOWN;
}

unsigned char next_rule(struct http_request *req, struct http_response *res, void *data){
  return DETECT_NEXT_RULE;
}



unsigned char detected_fail(struct http_request *req, struct http_response *res, void *data){
  return DETECT_FAIL;
}


struct ast_node *compile(char *string){
  parsed_ast=NULL;
  to_compile=string;
  globalReadOffset=0;
  yyparse();
  return parsed_ast;
}

int readInputForLexer( char *buffer, int *numBytesRead, int maxBytesToRead ) {
  int numBytesToRead = maxBytesToRead;
  int bytesRemaining = strlen(to_compile)-globalReadOffset;
  int i;
  if ( numBytesToRead > bytesRemaining ) { numBytesToRead = bytesRemaining; }
  for ( i = 0; i < numBytesToRead; i++ ) {
    buffer[i] = to_compile[globalReadOffset+i];
  }
  *numBytesRead = numBytesToRead;
  globalReadOffset += numBytesToRead;
  return 0;
}


void dump_sig(struct http_sig* sig){
  printf("SIG:\n");
  int i;
  for (i=0;i<FP_SIZE;i++) {
    printf("bucket %d: %d\n", i, sig->data[i]);
  }
}


void register_matching(){
  static const struct luaL_reg ruleset_m [] = {
    {"new", l_new_ruleset},
    {"new_rule", l_new_rule},
    {NULL, NULL}
  };
  register_faster_funcs("ruleset", ruleset_m);
}

int l_new_ruleset(luaState *L){
  struct match_ruleset *m=(struct match_ruleset *) lua_newuserdata(L, sizeof(struct match_ruleset));
  get_faster_value(L, "ruleset");
  lua_setmetatable(L, -2);
  return 1;
}



static struct target *check_ruleset(lua_State *L){
   void *ud = luaL_checkudata(L, 1, "faster.ruleset");
   luaL_argcheck(L, ud != NULL, 1, "`faster.ruleset' expected");
   return ((struct target *) ud);
}

int same_page(struct http_sig* sig1, struct http_sig* sig2) {
  u32 i, bucket_fail = 0;
  s32 total_diff  = 0;
  u32 total_scale = 0;
  /* Different response codes: different page */
  if (sig1->code != sig2->code)
    return 0;

  /* One has text and the other hasnt: different page */
  if (sig1->has_text != sig2->has_text)
    return 0;

  for (i=0;i<FP_SIZE;i++) {
    s32 diff = sig1->data[i] - sig2->data[i];
    u32 scale = sig1->data[i] + sig2->data[i];

    if (abs(diff) > 1 + (scale * FP_T_REL / 100) ||
        abs(diff) > FP_T_ABS)
      if (++bucket_fail > FP_B_FAIL) return 0;

    total_diff     += diff;
    total_scale    += scale;

  }

  if (abs(total_diff) > 1 + (total_scale * FP_T_REL / 100))
    return 0;
  return 1;
}



struct match_rule *new_rule(struct match_ruleset *ruleset){
  struct match_rule *rule;
  rule=calloc(sizeof(struct match_rule),1);
  rule->size=-1;
  rule->next=ruleset->head;
  ruleset->head=rule;
  rule->evaluate=next_rule;
  return rule;
}

int run_rules(struct match_ruleset *ruleset, struct http_request *req, struct http_response *res){
  struct match_rule *rule;
  int rc=DETECT_NEXT_RULE;
  for(rule=ruleset->head;rule!=NULL;rule=rule->next){
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
  if(rule->mime_type!=DETECT_ANY && res->header_mime && strcmp(rule->mime_type, (char *) res->header_mime)) return 0;
  if(rule->test_flags!=DETECT_ANY && req->test && !(req->test->flags & rule->test_flags)) return 0;
  
  if(rule->hash!=DETECT_ANY 
    && res
    && (!not_head_method(req) ||
    (not_head_method(req) 
    && res->md5_digest
    && memcmp(res->md5_digest,rule->hash,MD5_DIGEST_LENGTH)))) return 0; 

  if(rule->pattern!=DETECT_ANY && regexec(rule->pattern, (char *) serialize_path(req,0,0), 0, NULL, 0)) return 0;
  if(rule->sig!=DETECT_ANY && res && (!not_head_method(req) || (not_head_method(req) && (res->pay_len<50 || !same_page(rule->sig,&res->sig))))) return 0; 
  return 1;
}
