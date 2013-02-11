
#define OPT_USE_SSL                 (1 >> 1)
#define OPT_EXPECT_NO_BODY          (1 >> 2)

typedef struct prepared_http_request prepared_http_request;
struct prepared_http_request {
  simple_buffer *payload;
  simple_buffer *z_address;
  unsigned int handle;
  unsigned int options;
  prepared_http_request *next;
  prepared_http_request *prev;
  connection *conn;
  server_endpoint *endpoint;
};
