
enum parse_response_code {INVALID, NEED_MORE, FINISHED};
typedef struct http_response http_response;
struct http_response {
  unsigned int code;
  unsigned int must_close;
  unsigned int compressed;
  unsigned int chunked;
  int expected_body_len;
  char *body_ptr;
  size_t body_len;
  int chunked_length;

};
