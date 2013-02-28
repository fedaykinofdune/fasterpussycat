#ifndef SHOGGOTH_HTTP_RESPONSE_H
#define SHOGGOTH_HTTP_RESPONSE_H
#include <stdint.h>

#include "common/simple_buffer.h"


typedef struct http_response http_response;


#include "connection.h"



enum parse_response_code {INVALID, NEED_MORE, FINISHED, CONTINUE};
struct http_response {
  uint16_t code;
  uint8_t must_close;
  uint8_t compressed;
  uint8_t chunked;
  uint8_t expect_crlf;
  int expected_body_len;
  char *body_ptr;
  size_t body_offset;
  size_t body_len;
  simple_buffer *headers;
  uint32_t chunk_length;

};

/* http_response.c */
void reset_http_response(http_response *response);
http_response *alloc_http_response(void);
enum parse_response_code parse_connection_response(connection *conn);

#endif
