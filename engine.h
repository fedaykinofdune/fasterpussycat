#define MAX_404_QUERIES 5

#define DETECT_404_NONE 0
#define DETECT_404_CODE 1
#define DETECT_404_LOCATION 2

#define MAX_REPEATED_SIZE_COUNT 10


extern int check_dir;
extern int check_cgi_bin;
extern int check_404_size;
extern unsigned int max_requests;
extern int train;
extern unsigned int max_train_count;
extern int force_save;
struct feature_node {
  struct feature *data;
  struct feature_node *prev;
  struct feature_node *next;
};



struct test_score {
  struct url_test *test;
  double score;
  struct test_score *next;
};

struct target {
  unsigned char *host;
  unsigned char fourohfour_response_count;
  unsigned char fourohfour_detect_mode;
  unsigned int  fourohfour_content_length;
  unsigned int  twohundred_size;
  unsigned int  twohundred_size_count;
  unsigned char *fourohfour_location;
  unsigned int  last_code;
  int checks;
  unsigned char skip_dir;
  unsigned char skip_cgi_bin;
  struct http_request *prototype_request;
  struct feature_node *features;
  struct test_score  *test_scores;
  UT_hash_handle hh;
};

/* engine.c */
struct target *target_by_host(unsigned char *host);
void add_feature_label_to_target(const char *label, struct target *t);
void maybe_enqueue_tests(struct target *t);
void add_feature_to_target(struct feature *f, struct target *t);
void process_features(struct http_response *rep, struct target *t);
unsigned char process_test_result(struct http_request *req, struct http_response *rep);
int is_404(struct http_response *rep, struct target *t);
void add_target(unsigned char *host);
void gen_random(unsigned char *s, const int len);
unsigned char process_first_page(struct http_request *req, struct http_response *rep);
int process_404_responses(struct target *t);
int score_sort(struct test_score *lhs, struct test_score *rhs);
void enqueue_tests(struct target *t);
unsigned char process_random_request(struct http_request *req, struct http_response *rep);
unsigned char process_dir_request(struct http_request *req, struct http_response *rep);
void enqueue_random_request(struct target *t, int slash, int php, int dir, int pl);
