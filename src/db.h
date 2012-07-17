#ifndef FASTERPUSSYCAT_DB_H
#define FASTERPUSSYCAT_DB_H

#include "uthash.h"
#include "http_client.h"
/* db.c */

#define F_CRITICAL  (1 << 0) /* 0x01 */
#define F_DIRECTORY (1 << 1) /* 0x02 */
#define F_INFO (1 << 2) /* 0x04 */
#define F_CGI (1 << 3) /* 0x08 */

struct target;

struct feature_selection;

struct feature_selection {
  struct feature *feature;
  struct feature_selection *next;
};

struct trigger {
  int id;
  char *trigger;
  int feature_id;
  UT_hash_handle hh;
};

struct feature {
  int id;
  char *label;
  int count;
  int dirty;
  UT_hash_handle hh;

  UT_hash_handle hhl;
};

struct feature_test_result {
  int id;
  int url_test_id;
  int feature_id;
  int success;
  int count;
  int dirty;
  UT_hash_handle hh;
};

struct ftr_lookup_key {
  int url_test_id;
  int feature_id;
};

struct dir_link;

struct url_test {
  int id;
  char *url;
  char *description;
  int success;
  int count;
  int dirty;
  struct dir_link *parent;
  struct dir_link *children;
  struct feature_selection *feature_selections;
  unsigned int flags;
  UT_hash_handle hh;
};


struct req_pointer;
struct http_request;

struct req_pointer {
  struct http_request *req;
  struct dir_link *link;
  struct dir_link_res *parent;
  int success;
  int tested;
  int done;
  struct req_pointer *next;
};

struct dir_link_res {
  int parent_id;
  int success;
  int tested;
  struct req_pointer *children;
  UT_hash_handle hh;
};

struct dir_link {
  int id;
  struct url_test *parent;
  struct url_test *child;
  int parent_id;
  int child_id;
  int count;
  int child_success;
  int parent_success;
  int parent_child_success;
  int dirty;
  struct dir_link *next;
  UT_hash_handle hh;
};

void update_feature_selection(struct url_test *test);

void load_feature_selections();
struct feature *get_features(void);
int open_database(void);
void load_aho_corasick_triggers(void);
void add_features_from_triggers(struct http_response *rep, struct target *t);
void add_aho_corasick_trigger(char *trigger, char *feature);
struct feature *find_or_create_feature_by_label(const char *label);
void save_feature(struct feature *f);
void save_all(void);
void save_ftr(struct feature_test_result *f);

void create_dir_links();
void save_dir_link(struct dir_link *link);
void save_url_test(struct url_test *f);
struct feature_test_result *find_or_create_ftr(int url_test_id, int feature_id);
int load_ftr_by_feature_id(int feature_id);
int load_features(void);
int load_feature(void);
int load_ftr(void);
int load_tests(void);
struct url_test *get_tests(void);
void add_or_update_url(char *url, char *description, unsigned int flags);
#endif
