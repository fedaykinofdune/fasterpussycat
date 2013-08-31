typedef struct {
  unsigned int handle;
  shog_handler_t handler;
  void *data;
  unsigned int request_type;
  union {
    prepared_http_request_t *preq;
    http_request_t *req;
  };
  UT_hash_handle hh;
} promise_t;
