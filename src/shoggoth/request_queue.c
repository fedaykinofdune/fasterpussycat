#include <netinet/in.h>
#include "server_endpoint.h"
#include "connection_pool.h"


server_endpoint *endpoint_queue_head=NULL;
server_endpoint *endpoint_queue_tail=NULL;
server_endpoint *endpoint_working_head=NULL;
server_endpoint *server_endpoint_map=NULL;


inline request_queue_t *request_queue_alloc(tcp_endpoint_t *tcp_endpoint){
  request_queue_t *queue=malloc(sizeof(request_queue_t));
  queue->idle_list=NULL;
  queue->conn_list=NULL;
  queue->next=NULL;
  queue->prev=NULL;
  queue->errors=0;  
  queue->queue_head=NULL;
  queue->queue_tail=NULL;
  queue->next_working=NULL;
  queue->prev_working=NULL;
  queue->working=0;
  memcpy(&(endpoint->tcp_endpoint), tcp_endpoint, sizeof(tcp_endpoint);
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

void request_queue_destroy(request_queue_t *queue){
  printf("destroying endpoint %p\n", endpoint);
  connection_t *current;
  HASH_DEL(request_queue_map, queue);
  switch(queue->working){
    
    case 0:
      if(queue==request_queue_queue_head) request_queue_queue_head=queue->next_queue;
      if(queue==request_queue_queue_tail) request_queue_queue_tail=queue->prev_queue;
      if(queue->prev_queue) queue->prev_queue->next_queue=queue->next_queue;
      if(queue->next_queue) queue->next_queue->prev_queue=queue->prev_queue;
      break;
    case 1:
      n_hosts--;
      if(queue->prev_working) queue->prev_working->next_working=queue->next_working;
      if(queue->next_working) queue->next_working->prev_working=queue->prev_working;
      if(queue==request_queue_working_head) request_queue_working_head=queue->next_working;
      for(current=queue->conn_list; current; current=current->next_conn){
        connection_disassociate_from_endpoints(current);
      }
      break;
  }
  free(queue);
}



void request_queue_enqueue(prepared_http_request *preq){
  
  request_queue_t *queue=request_queue_find_or_create(preq->tcp_endpoint);
  preq->queue=queue; 
  if(queue->queue_tail){
      queue->queue_tail->next=preq;
      preq->prev=queue->queue_tail;
  }
  else queue->queue_head=preq;

  queue->queue_tail=preq;
  
}



inline request_queue_t *request_queue_find_or_create(tcp_endpoint_t *t){
  request_queue_t *queue;
  HASH_FIND(hh, request_queue_map, t, sizeof(tcp_endpoint_t), queue);
  if(queue) return queue;

  queue=request_queue_alloc(t);
  HASH_ADD(hh, request_queue_map, tcp_endpoint, sizeof(struct tcp_endpoint_t), queue);
  if(!request_queue_queue_head) request_queue_queue_head=queue;
  else{
    request_queue_queue_tail->next_queue=queue;
    queue->prev_queue=request_queue;
  }
  endpoint_queue_tail=endpoint;
  return endpoint;
}




