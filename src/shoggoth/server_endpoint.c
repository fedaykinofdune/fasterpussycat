#include <netinet/in.h>
#include "server_endpoint.h"
#include "connection_pool.h"


server_endpoint *endpoint_queue_head=NULL;
server_endpoint *endpoint_queue_tail=NULL;
server_endpoint *endpoint_working_head=NULL;
server_endpoint *server_endpoint_map=NULL;


inline server_endpoint *alloc_server_endpoint(struct sockaddr_in *addr){
  server_endpoint *endpoint=malloc(sizeof(server_endpoint));
  endpoint->idle_list=NULL;
  endpoint->conn_list=NULL;
  endpoint->next=NULL;
  endpoint->prev=NULL;
  endpoint->errors=0;  
  endpoint->queue_head=NULL;
  endpoint->queue_tail=NULL;
  endpoint->next_working=NULL;
  endpoint->prev_working=NULL;
  endpoint->addr=malloc(sizeof(struct sockaddr_in));
  memcpy(endpoint->addr,addr,sizeof(struct sockaddr_in));
  return endpoint;
}

void debug_print_endpoint_queue(server_endpoint *endpoint){
  prepared_http_request *r;
  printf("\nEndpoint queue:\n");
  for(r=endpoint->queue_head;r;r=r->next){
    printf(" >> %p\n",r);
  }
  printf("-------------------\n");
}


void debug_print_endpoint_queue_length(server_endpoint *endpoint){
  prepared_http_request *r;
  int count=0;
  for(r=endpoint->queue_head;r;r=r->next){
    count++;
  }
  printf("queue length: %d\n",count);
}

void destroy_server_endpoint(server_endpoint *endpoint){
  printf("destroying endpoint %p\n", endpoint);
  connection *current;
  HASH_DEL(server_endpoint_map, endpoint);
  if(endpoint==endpoint_queue_head) endpoint_queue_head=endpoint->next;
  if(endpoint==endpoint_queue_tail) endpoint_queue_tail=endpoint->prev;
  if(endpoint->prev) endpoint->prev->next=endpoint->next;
  if(endpoint->next) endpoint->next->prev=endpoint->prev;
  if(endpoint->prev_working) endpoint->prev_working->next_working=endpoint->next_working;
  if(endpoint->next_working) endpoint->next_working->prev_working=endpoint->prev_working;
  if(endpoint->prev_working || endpoint_working_head==endpoint) n_hosts--;
  if(endpoint==endpoint_working_head) endpoint_working_head=endpoint->next_working;
  for(current=endpoint->conn_list; current; current=current->next_conn){
    disassociate_connection_from_endpoints(current);
  }
  free(endpoint->addr);
  free(endpoint);
}

server_endpoint *find_or_create_server_endpoint(struct sockaddr_in *addr){
  server_endpoint *endpoint;
  HASH_FIND(hh, server_endpoint_map, addr, sizeof(struct sockaddr_in), endpoint);
  if(endpoint) return endpoint;

  endpoint=alloc_server_endpoint(addr);
  printf("ednpoint %p\n", endpoint);
  printf("endpoint addr %p", endpoint->addr);
  HASH_ADD_KEYPTR(hh, server_endpoint_map, endpoint->addr, sizeof(struct sockaddr_in), endpoint);
  if(!endpoint_queue_head) endpoint_queue_head=endpoint;
  if(endpoint_queue_tail) {
    endpoint_queue_tail->next=endpoint;
    endpoint->prev=endpoint_queue_tail;
  }
  endpoint_queue_tail=endpoint;
  return endpoint;
}




