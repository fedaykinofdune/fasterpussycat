
#ifndef FASTERPUSSYCAT_ENGINE_H
#define FASTERPUSSYCAT_ENGINE_H

#define MAX_404_QUERIES 5

#define DETECT_404_NONE 0
#define DETECT_404_CODE 1
#define DETECT_404_LOCATION 2

#define MAX_REPEATED_SIZE_COUNT 10

#include "http_client.h"
#include "db.h"
#include "uthash.h"

extern int check_dir;
extern int check_cgi_bin;
extern int check_404_size;
extern unsigned int max_requests;
extern int train;
extern unsigned int max_train_count;
extern int force_save;
extern int scan_flags;


struct feature_node {
  struct feature *data;
  struct feature_node *prev;
  struct feature_node *next;
};

struct request_response; 

struct request_response {
  struct http_request  *req;
  struct http_response *res;
  struct request_response *next;
};

struct test_score {
  struct url_test *test;
  double score;
  int no_url_save;
  struct test_score *next;
};

struct target {
  unsigned char *host;
  unsigned char skip_dir;
  unsigned char skip_cgi_bin;
  unsigned char *full_host;
  void (*after_probes) (struct target *t);
  struct http_request *prototype_request;
  struct feature_node *features;
  struct test_score  *test_scores;
  struct detect_404_info *detect_404;
  UT_hash_handle hh;
  struct request_response *results;
  struct dir_link_res *link_map;
  int requests;
  int requests_left;
  char *upload_file;
};

/* engine.c */
struct target *get_targets();
unsigned char *macro_expansion(const unsigned char *url);
void output_result(struct http_request *req, struct http_response *res);
struct target *target_by_host(unsigned char *host);
void add_feature_label_to_target(const char *label, struct target *t);
void maybe_enqueue_tests(struct target *t);
void add_feature_to_target(struct feature *f, struct target *t);
void process_features(struct http_response *rep, struct target *t);
unsigned char process_test_result(struct http_request *req, struct http_response *rep);
struct target *add_target(unsigned char *host);
void gen_random(unsigned char *s, const int len);
unsigned char process_first_page(struct http_request *req, struct http_response *rep);
int score_sort(struct test_score *lhs, struct test_score *rhs);
void enqueue_tests(struct target *t);
void enqueue_random_request(struct target *t, int slash, int php, int dir, int pl);
void maybe_update_dir_link(struct req_pointer *pointer);
struct http_request *new_request(struct target *t);
struct http_request *new_request_with_method(struct target *t, unsigned char *method);
struct http_request *new_request_with_method_and_path(struct target *t, unsigned char *method, unsigned char *path);
void destroy_target(struct target *t);
#endif
