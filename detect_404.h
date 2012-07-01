#define CHECK_EXT {"php", "html", "asp", "idq"}
#define CHECK_EXT_LEN 4

#define USE_404 = 1
#define USE_HASH = 2
#define USE_SIG = 3

#define PROBE_GENERAL;

struct detect_404_cleanup_info;

struct detect_404_cleanup_info{
  void (*cleanup_func) (void *data);
  void *data;
  struct detect_404_cleanup *next;
}

struct detect_404_info {
  int probes;
  struct match_rule *rules_preprocess;
  struct match_rule *rules_general;
  struct match_rule *rules_final;
  struct detect_404_cleanup_info *cleanup;
  struct request_response *general_probes;  
  struct match_rule *recommended_request_methods;  
};

struct probe {
  int type;
  int count;
  char *data;
  struct request_response *responses;  
};

unsigned char detected_success(struct http_request *req, struct http_response *res, void *data);
unsigned char detected_404(struct http_request *req, struct http_response *res, void *data);
void *detect_404_alloc(struct detect_404_info *info, size_t size);
void detect_404_cleanup(struct detect_404_info);

