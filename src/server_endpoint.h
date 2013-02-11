typedef struct server_endpoint server_endpoint;
extern server_endpoint *server_endpoint_map;
extern server_endpoint *endpoint_queue_head;
extern server_endpoint *endpoint_queue_tail;
extern server_endpoint *endpoint_working_head;

struct server_endpoint {
  sockaddr_in addr;
  prepared_http_request *queue_head;
  prepared_http_request *queue_tail;
  unsigned char use_ssl;
  connection *conn_list;
  connection *idle_list;

  /* doublely linked list */

  server_endpoint *next;
  server_endpoint *prev;


  server_endpoint *next_working;
  server_endpoint *prev_working;
}

