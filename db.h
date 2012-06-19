#include "uthash.h"
/* db.c */

#define CODE_200 (1 << 0) /* 0x01 */
#define CODE_301 (1 << 1) /* 0x02 */
#define CODE_302 (1 << 2) /* 0x04 */
#define CODE_401 (1 << 3) /* 0x08 */
#define CODE_403 (1 << 4) /* 0x10 */
#define CODE_500 (1 << 5) /* 0x20 */
#define CODE_OTHER (1 << 7) /* 0x80 */

#define ALL_CODES (CODE_200 | CODE_301 | CODE_302 | CODE_401 | CODE_403 | CODE_500 | CODE_OTHER)



#define F_CRITICAL  (1 << 0) /* 0x01 */
#define F_DIRECTORY (1 << 1) /* 0x02 */
#define F_INFO (1 << 2) /* 0x04 */
#define F_CGI (1 << 3) /* 0x08 */



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
  int code_200;
  int code_301;
  int code_302;
  int code_500;
  int code_403;
  int code_401;
  int code_other;
  int count;
  int dirty;
  UT_hash_handle hh;
};

struct ftr_lookup_key {
  int url_test_id;
  int feature_id;
};

struct url_test {
  int id;
  char *url;
  char *description;
  int code_200;
  int code_301;
  int code_302;
  int code_500;
  int code_403;
  int code_401;
  int code_other;
  int count;
  int dirty;
  unsigned int flags;
  UT_hash_handle hh;
};



int open_database(void);
void load_aho_corasick_triggers(void);
void add_features_from_triggers(struct http_response *rep, struct target *t;);
void add_aho_corasick_trigger(char *trigger, char *feature);
struct feature *find_or_create_feature_by_label(const char *label);
void save_feature(struct feature *f);
void save_all(void);
void save_ftr(struct feature_test_result *f);
void save_url_test(struct url_test *f);
struct feature_test_result *find_or_create_ftr(int url_test_id, int feature_id);
int load_ftr_by_feature_id(int feature_id);
int load_features(void);
int load_feature(void);
int load_ftr(void);
int load_tests(void);
struct url_test *get_tests(void);
void add_or_update_url(char *url, char *description, unsigned int flags);
