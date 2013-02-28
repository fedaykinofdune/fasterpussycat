#ifndef SHOGGOTH_SERVER_ENDPOINT_H
#define SHOGGOTH_SERVER_ENDPOINT_H

#include <sys/types.h>          
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct server_endpoint server_endpoint;

#include "common/uthash.h"
#include "connection.h"
#include "prepared_http_request.h"

extern server_endpoint *server_endpoint_map;
extern server_endpoint *endpoint_queue_head;
extern server_endpoint *endpoint_queue_tail;
extern server_endpoint *endpoint_working_head;

struct server_endpoint {
  struct sockaddr_in *addr;
  prepared_http_request *queue_head;
  prepared_http_request *queue_tail;
  unsigned char use_ssl;
  connection *conn_list;
  connection *idle_list;

  /* doublely linked list */

  server_endpoint *next;
  server_endpoint *prev;
  unsigned int errors;

  server_endpoint *next_working;
  server_endpoint *prev_working;
  UT_hash_handle hh;
};

/* server_endpoint.c */
void destroy_server_endpoint(server_endpoint *endpoint);
server_endpoint *find_or_create_server_endpoint(struct sockaddr_in *addr);


#endif
