#define SHOG_OK 0
#define SHOG_ERR_CONN 1
#define SHOG_ERR_HTTP 2
#define SHOG_ERR_TIME 3

enum shoggoth_request_type { NORMAL, PREPARED};

typedef struct {
  int status;
  union {
    prepared_http_request *preq;
    http_request *req;
  };
  shoggoth_request_type *req_type;
  http_response *resp;
} shog_http_event_t;


typedef void (*) (shog_http_event_t *, void *data) shog_handler_t;

