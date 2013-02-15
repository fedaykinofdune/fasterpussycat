#ifndef SHOGGOTH_HTTP_REQUEST_H
#define SHOGGOTH_HTTP_REQUEST_H

#include "simple_buffer.h"

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

#endif
