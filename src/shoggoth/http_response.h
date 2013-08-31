typedef struct {
  uint16_t status;
  uint16_t code;
  simple_buffer_t body;
  http_header_t *headers;

  /* internal use only */

  simple_buffer_t *packed_headers;

} http_response_t;
