#include <openssl/md5.h>
#include <sys/types.h>
#include <regex.h>
#define CHECK_EXT {"php", "html", "asp", "idq","jsp"}
#define CHECK_EXT_LEN 5

#define USE_404  1
#define USE_HASH  2
#define USE_SIG  3
#define USE_UNKNOWN 4

#define PROBE_GENERAL 1
#define PROBE_EXT 2
#define PROBE_DIR 3
#define PROBE_CGI 4

#define RECOMMEND_SKIP 0

extern int blacklist_success;
extern int skip_other_probes;

struct detect_404_cleanup_info;

struct detect_404_cleanup_info {
  void (*cleanup_func) (void *data);
  void *data;
  struct detect_404_cleanup_info *next;
};

struct recommended_method;

struct recommended_method {
  unsigned char *method;
  int flags;
  regex_t *pattern;
  struct recommended_method *next;
};

struct hash_count {
  unsigned char hash[MD5_DIGEST_LENGTH];
  unsigned int count;
  UT_hash_handle hh; 
};

struct detect_404_info {
  int probes;
  struct match_rule *rules_preprocess;
  struct match_rule *rules_general;
  struct match_rule *rules_final;
  struct detect_404_cleanup_info *cleanup;
  struct request_response *general_probes;  
  struct recommended_method *recommended_request_methods;  
  struct hash_count *hash_counts;
  unsigned int success;
  unsigned int count;
  unsigned int removed;
  unsigned int in_a_row_dir;
  unsigned int last_dir_code;
  unsigned int dir_blacklist;
};



struct probe {
  int type;
  int count;
  char *data;
  struct request_response *responses;  
};

/* detect_404.c */
void gen_random(unsigned char *s, const int len);
void add_default_rules(struct detect_404_info *info);
unsigned char detect_error(struct http_request *req, struct http_response *res, void *data);
char *recommend_method(struct detect_404_info *info, struct url_test *test);
struct match_rule *new_404_rule(struct detect_404_info *info, struct match_rule **list);
void *detect_404_alloc(struct detect_404_info *info, size_t size);
void detect_404_cleanup(struct detect_404_info *info);
int determine_404_method(struct detect_404_info *info, struct request_response *list);
void blacklist_hash(struct detect_404_info *info, unsigned char *hash);
void blacklist_sig(struct detect_404_info *info, struct http_sig *sig);
void enqueue_other_probes(struct target *t);
u8 process_probe(struct http_request *req, struct http_response *res);
void launch_404_probes(struct target *t);
void destroy_detect_404_info(struct detect_404_info *info);
int is_404(struct detect_404_info *info, struct http_request *req, struct http_response *res);
unsigned char enforce_magic_rule(struct http_request *req, struct http_response *res, void *data);
void create_magic_rules(struct detect_404_info *info);
void create_magic_rule(struct detect_404_info *info, char *ext, char *mime_type);
