#ifndef SHOGGOTH_PREPARED_HTTP_REQUEST_H
#define SHOGGOTH_PREPARED_HTTP_REQUEST_H


typedef struct prepared_http_request prepared_http_request;

#include "connection.h"
#include "server_endpoint.h"

struct prepared_http_request {
  simple_buffer *payload;
  char z_address[8];
  size_t z_address_size;
  int32_t handle;
  unsigned int options;
  unsigned short port;
  prepared_http_request *next;
  prepared_http_request *prev;
  connection *conn;
  server_endpoint *endpoint;
};

/* prepared_http_request.c */
prepared_http_request *alloc_prepared_http_request(void);
void destroy_prepared_http_request(prepared_http_request *p);

#endif
