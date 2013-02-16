#ifndef SHOGGOTH_HTTP_REQUEST_H
#define SHOGGOTH_HTTP_REQUEST_H

#include "simple_buffer.h"
#include "prepared_http_request.h"

typedef struct {

  simple_buffer *headers;
  simple_buffer *path;
  simple_buffer *method;
  simple_buffer *body;
  simple_buffer *host;
  simple_buffer *z_address;
  unsigned short port;
  unsigned int options;
  unsigned int handle;
} http_request;

/* http_request.c */
void write_http_request_headers_to_simple_buffer(simple_buffer *buf, const http_request *req);
void write_http_request_to_simple_buffer(simple_buffer *buf, const http_request *req);
void enqueue_request(http_request *req);
prepared_http_request *prepare_http_request(http_request *req);

#endif
