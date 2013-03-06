#include "global_options.h"
#include "server_endpoint.h"
#include "prepared_http_request.h"
#include "connection_pool.h"

prepared_http_request *alloc_prepared_http_request(){
  prepared_http_request *p=malloc(sizeof(prepared_http_request));
  p->next=NULL;
  p->conn=NULL;
  p->prev=NULL;
  p->endpoint=NULL;
  p->handle=0;
  p->payload=alloc_simple_buffer(opt.request_buffer_size);
  return p;
}

void destroy_prepared_http_request(prepared_http_request *p){
  if(p->endpoint){
    server_endpoint *endpoint=p->endpoint;
       
    if(endpoint->queue_head==p) endpoint->queue_head=p->next;
    if(endpoint->queue_tail==p) endpoint->queue_tail=p->prev;
    if(p->prev) p->prev->next=p->next;
    if(p->next) p->next->prev=p->prev;
  }
  if(p->conn){
    connection *conn=p->conn;
    conn->request=NULL;
    conn->old_state=p->conn->state;
    conn->state=IDLE;
    if(conn==active_connections) {active_connections=conn->next_active;}
    if(conn->next_active) conn->next_active->prev_active=conn->prev_active;
    if(conn->prev_active) conn->prev_active->next_active=conn->next_active;
    conn->next_active=NULL;
    conn->prev_active=NULL;
  }
  if(p->conn && p->endpoint){
    p->conn->next_idle=p->endpoint->idle_list;
    p->endpoint->idle_list=p->conn;
    
  }

  p->conn=NULL;
  destroy_simple_buffer(p->payload);
  free(p);
}
