#ifndef SHOGGOTH_SERVER_ENDPOINT_H
#define SHOGGOTH_SERVER_ENDPOINT_H

#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct request_queue request_queue_t;

#include "common/uthash.h"
#include "connection.h"
#include "prepared_http_request.h"

extern request_queue_t *request_queue_map;
extern request_queue_t *request_queue_queue_head;
extern request_queue_t *request_queue_queue_tail;
extern request_queue_t *request_queue_working;

struct request_queue_t {
  tcp_endpoint_t tcp_endpoint;
  prepared_http_request *queue_head;
  prepared_http_request *queue_tail;
  connection *conn_list;
  connection *idle_list;

  /* doublely linked list */

  request_queue_t *next_queue;
  request_queue_t *prev_queue;
  unsigned int errors;
  
  request_queue_t *next_working;
  request_queue_t *prev_working;
  UT_hash_handle hh;
};

/* server_endpoint.c */
void destroy_server_endpoint(server_endpoint *endpoint);
server_endpoint *find_or_create_server_endpoint(struct sockaddr_in *addr);
void debug_print_endpoint_queue(server_endpoint *endpoint);
void debug_print_endpoint_queue_length(server_endpoint *endpoint);
#endif
