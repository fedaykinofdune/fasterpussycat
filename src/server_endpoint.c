#include "server_endpoint.h"

server_endpoint *endpoint_queue_head=NULL;
server_endpoint *endpoint_queue_tail=NULL;
server_endpoint *endpoint_working_head=NULL;
server_endpoint *server_endpoint_map=NULL;

inline server_endpoint *alloc_server_endpoint(struct *sockaddr_in addr){
  server_endpoint *endpoint=malloc(sizeof(server_endpoint));
  endpoint->next_conn=NULL;
  endpoint->next_idle=NULL;
  endpoint->next=NULL;
  endpoint->prev=NULL;
  
  endpoint->next_working=NULL;
  endpoint->prev_working=NULL;
  endpoint->addr=alloc(sizeof(addr));
  memcpy(endpoint->addr,addr,sizeof(addr));
  return endpoint;
}

void destroy_server_endpoint(server_endpoint *endpoint){
  if(endpoint==endpoint_queue_head) endpoint_queue_head=endpoint->next;
  if(endpoint==endpoint_queue_tail) endpoint_queue_tail=endpoint->prev;
  if(endpoint->prev) endpoint->prev->next=endpoint->next;
  if(endpoint->next) endpoint->next->prev=endpoint->prev;
  if(endpoint==endpoint_working_head) endpoint_working_head=endpoint->next_working;
  if(endpoint->prev_working) endpoint->prev_working->next_working=endpoint->next_working;
  if(endpoint->next_working) endpoint->next_working->prev_working=endpoint->prev_working;
  if(endpoint->prev_working || endpoint_working_head==endpoint) n_hosts--;
  for(connection *current=endpoint->next_conn; current; current=current->next_conn){
    dissassocate_connection_from_endpoint(current);
  }
  free(endpoint->addr);
  free(endpoint);
}

server_endpoint *find_or_create_server_endpoint(struct *sockaddr_in addr){
  server_endpoint *endpoint;
  HASH_FIND(hh, server_endpoint_map, addr, sizeof(addr), endpoint);
  if(endpoint) return endpoint;

  endpoint=alloc_server_endpoint(addr);
  HASH_ADD(hh, server_endpoint_map, endpoint->addr, sizeof(addr), endpoint);
  if(!endpoint_queue_head) endpoint_queue_head=endpoint;
  if(endpoint_queue_tail) {
    endpoint_queue_tail->next=endpoint;
    endpoint->prev=endpoint_queue_tail;
  }
  endpoint_queue_tail=endpoint;
  return endpoint;
}




