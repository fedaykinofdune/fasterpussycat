#ifndef FASTERPUSSYCAT_MATCH_RULE_H
#define FASTERPUSSYCAT_MATCH_RULE_H


#include "http_client.h"
#include "ast.h"
#include "lua.h"

#define DETECT_FAIL 0
#define DETECT_SUCCESS 1
#define DETECT_NEXT_RULE 2
#define DETECT_UNKNOWN 3

#define DETECT_ANY 0
#define MATCH_200 (1 << 0)
#define MATCH_404 (1 << 1)
#define MATCH_403 (1 << 2)
#define MATCH_401 (1 << 3)
#define MATCH_500 (1 << 4)
#define MATCH_301 (1 << 5)
#define MATCH_302 (1 << 6)

#include <sys/types.h>
#include <regex.h>
#include <openssl/md5.h>

struct match_rule;


struct match_rule {
  struct ast_node *ast;
  char *code;
  void *data;
  unsigned char (*evaluate) (struct http_request *req, struct http_response *rep, void *data);
  struct match_rule *next;
};

struct match_ruleset {
  struct match_rule *head;
};

/* match_rule.c */
unsigned char detected_success(struct http_request *req, struct http_response *res, void *data);
unsigned char detected_unknown(struct http_request *req, struct http_response *res, void *data);
unsigned char next_rule(struct http_request *req, struct http_response *res, void *data);
unsigned char detected_fail(struct http_request *req, struct http_response *res, void *data);
struct ast_node *compile_ast(const char *string);
int readInputForLexer(char *buffer, int *numBytesRead, int maxBytesToRead);
void dump_sig(struct http_sig *sig);
void register_matching(void);
int l_new_ruleset(lua_State *L);
int l_new_rule(lua_State *L);
int same_page(struct http_sig *sig1, struct http_sig *sig2);
struct match_rule *new_rule(struct match_ruleset *ruleset);
int run_rules(struct match_ruleset *ruleset, struct http_request *req, struct http_response *res);
int not_head_method(struct http_request *req);
int rule_matches(struct match_rule *rule, struct http_request *req, struct http_response *res);

#endif
