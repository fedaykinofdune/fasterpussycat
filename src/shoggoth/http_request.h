#ifndef SHOGGOTH_HTTP_REQUEST_H
#define SHOGGOTH_HTTP_REQUEST_H

#include "common/simple_buffer.h"
#include "prepared_http_request.h"

typedef struct {
  http_header_t *headers;
  simple_buffer_t *path;
  simple_buffer_t *body;
  
  uint16_t port;
  uint32_t options;
  uint32_t handle;
  uint8_t method;

  /* internal use */
  
  uint8_t host_set;
  uint8_t content_length_set;
} http_request_t;



/* http_request.c */
void http_request_write_headers_to_simple_buffer(simple_buffer *buf, const http_request_t *req);
void http_request_write_to_simple_buffer(simple_buffer *buf, const http_request_t *req);
void enqueue_request(http_request_t *req);
prepared_http_request *prepare_http_request(http_request_t *req);

#endif
