
#define DETECT_FAIL 0
#define DETECT_SUCCESS 1
#define DETECT_NEXT_RULE 2
#define DETECT_UNKNOWN 3

#define DETECT_ANY 0

struct match_rule;


struct match_rule {
  regex_t *pattern;
  unsigned char *method;
  struct http_sig *sig;
  unsigned int code;
  unsigned int size;
  unsigned int test_flags;
  char *hash;
  void *data;
  unsigned char (*evaluate) (struct http_request *req, struct http_response *rep, void *data);
  struct match_rule *next;
};
/* match_rule.c */
unsigned char detected_success(struct http_request *req, struct http_response *res, void *data);

unsigned char detected_unknown(struct http_request *req, struct http_response *res, void *data);
unsigned char next_rule(struct http_request *req, struct http_response *res, void *data);
unsigned char detected_fail(struct http_request *req, struct http_response *res, void *data);
int same_page(struct http_sig *sig1, struct http_sig *sig2);
int not_head_method(struct http_request *req);
struct match_rule *new_rule(struct match_rule **list);
int run_rules(struct match_rule *list, struct http_request *req, struct http_response *res);
int rule_matches(struct match_rule *rule, struct http_request *req, struct http_response *res);
