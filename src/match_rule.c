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


struct ast_node *compile_ast(char *string){
  parsed_ast=NULL;
  to_compile=string;
  globalReadOffset=0;
  if(!yyparse()) return NULL;
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
    {"add_rule", l_new_rule},
    {"__index", l_arb_getprop},
    {"__newindex", l_arb_setprop},
    {NULL, NULL}
  };
  register_faster_funcs("ruleset", ruleset_m);

  static const struct luaL_reg rule_m [] = {
    {"__tostring", l_rule_tostring},
    {NULL, NULL}
  };
  register_faster_funcs("rule", rule_m);

}

int l_new_ruleset(luaState *L){
  struct match_ruleset *m=(struct match_ruleset *) lua_newuserdata(L, sizeof(struct match_ruleset));
  get_faster_value(L, "ruleset");
  lua_setmetatable(L, -2);
  setup_obj_env();
  return 1;
}



static struct target *check_ruleset(lua_State *L){
   void *ud = luaL_checkudata(L, 1, "faster.ruleset");
   luaL_argcheck(L, ud != NULL, 1, "`faster.ruleset' expected");
   return ((struct match_ruleset *) ud);
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



static int l_new_rule(lua_State *L){
  struct match_ruleset *ruleset=check_ruleset(L,1);
  char *rule_code=luaL_checkstring(L,2);
  struct ast_node *compiled=compile_ast(rule_code);
  struct match_rule *match=NULL;
  if(!compiled){
    error="Error compiling rule";
    goto l_new_rule_error;
  }
  match=malloc(sizeof(struct match_rule));
  get_faster_value(L, "rule");
  lua_setmetatable(L, -2);
  match->ast=compiled;
  if(lua_isstring(L, 3)){
    func_s=lua_tostring(L,3);
    if(strcmp(func_s,"SUCC")){
      match->evaluate=detect_success;
    }
    else if(strcmp(func_s,"FAIL")){
      match->evaluate=detect_fail;
    }
    else{
      error="func must be SUCC, FAIL or callback";
      goto l_new_rule_error;
    }

  }
  else if(lua_isfunction(L,3)){
    callback=ck_malloc(sizeof(struct l_callback));
    lua_push(L,3);
    callback->callback_ref=luaL_ref(L, LUA_REGISTRYINDEX);
    callback->data_ref=LUA_NILREF;
    match->data=callback;
    match->evaluate=lua_callback_evaluate;
  }

  else{
    error="func must be SUCC, FAIL or callback";
    goto l_new_rule_error;
  }


  if(callback && !lua_isnil(4)) {
    lua_push(4,L);
    callback->data_ref=luaL_ref(L, LUA_REGISTRYINDEX);
  }
  match->rule_code=strdup(rule_code);
  match->next=ruleset->head;
  ruleset->head=match;
  lua_pushboolean(L,1);
  return 1;
l_new_rule_error:
  free(match);
  lua_pushnil();
  lua_pushstring(error);
  return 2;
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
  return ast_eval(rule->ast,req,res);;
}
