#ifndef SHOGGOTH_HTTP_RESPONSE_PARSER_H
#define SHOGGOTH_HTTP_RESPONSE_PARSER_H
#include <stdint.h>

#include "common/simple_buffer.h"


typedef struct http_parser http_parser_t;


#include "connection.h"



enum http_parser_state {INVALID, NEED_MORE, FINISHED, CONTINUE};

struct http_parser {
  uint16_t code;
  uint8_t must_close;
  uint8_t compressed;
  uint8_t chunked;
  uint8_t expect_crlf;
  int expected_body_len;
  char *body_ptr;
  size_t body_offset;
  size_t last_z_offset;
  size_t body_len;
  simple_buffer_t *packed_headers;
  uint32_t chunk_length;
  simple_buffer_t *read_buffer;
  simple_buffer_t *aux_buffer;
  z_stream z_stream;
  unsigned int z_stream_initialized;
};

/* http_response.c */
void http_parser_reset(http_parser_t *parser);
http_parser_t *http_parser_alloc(void);
enum http_parser_state http_parser_parse(http_parser_t *parser);

#endif
