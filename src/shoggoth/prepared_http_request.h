#ifndef SHOGGOTH_PREPARED_HTTP_REQUEST_H
#define SHOGGOTH_PREPARED_HTTP_REQUEST_H


typedef struct prepared_http_request prepared_http_request;

#include "connection.h"
#include "server_endpoint.h"

struct prepared_http_request {
  simple_buffer_t payload;
  char z_address[16];
  tcp_endpoint_t tcp_endpoint;
  size_t z_address_size;
  int32_t handle;
  unsigned int options;
  unsigned short port;
  zmq_msg_recv payload_msg;
  prepared_http_request_t *next;
  prepared_http_request_t *prev;
  connection_t *conn;
  request_queue_t *queue;
  
};

/* prepared_http_request.c */
prepared_http_request *alloc_prepared_http_request(void);
void destroy_prepared_http_request(prepared_http_request *p);

#endif
