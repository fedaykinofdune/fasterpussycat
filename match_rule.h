
#define DETECT_FAIL 0
#define DETECT SUCCESS 1
#define DETECT_NEXT_RULE 2
#define DETECT_UNKNOWN 3

#define DETECT_ANY 0

struct match_rule;


struct match_rule {
  regex_t *pattern;
  char *method;
  struct http_sig *sig;
  unsigned int code;
  unsigned int size;
  unsigned int test_flags;
  char *hash;
  void *data;
  unsigned char (*evaluate) (struct detect_404_info *info, struct http_request *req, struct http_response *rep, void *data);
  struct match_rule *next;
}
