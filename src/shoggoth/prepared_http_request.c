#include "shoggoth/global_options.h"
#include "shoggoth/tcp_endpoint.h""
#include "prepared_http_request.h"
#include "connection_pool.h"

static memory_pool_fixed_t *pool=NULL;


prepared_http_request_t *prepared_http_request_alloc(){
  if(!pool) pool=memory_pool_fixed_new(sizeof(prepared_http_request_t), 64);
  prepared_http_request_t *p=memory_pool_fixed_alloc(pool);
  p->next=NULL;
  p->conn=NULL;
  p->prev=NULL;
  p->endpoint=NULL;
  p->handle=0;
  p->queue=NULL;
  p->payload.size=0;
  p->payload.ptr=NULL;
  simple_buffer_reset( &(p->payload) );
  return p;
}



void prepared_http_request_reset(prepared_http_request *preq){;
  p->next=NULL;
  p->conn=NULL;
  p->prev=NULL;
  p->endpoint=NULL;
  p->handle=0;
  simple_buffer_reset( &(preq->payload) );
  p->queue=NULL;
}


void prepared_http_request_enqueue(prepared_http_request_t *preq){
  lookup_address(host,prepared_http_request_enqueue2, preq); 
}


void prepared_http_request_builder_add_method(prepared_http_request_t *preq, int method){
  simple_buffer_t *buf=preq->payload;
  switch(method){
    case GET:
      simple_buffer_write(buf, "GET", 3);
      break;
    case POST:
      simple_buffer_write(buf, "POST", 4);
      break;
    case PUT:
      simple_buffer_write(buf, "PUT", 3);
      break;
    case DELETE:
      simple_buffer_write(buf, "DELETE", 6);
      break;
    case CONNECT:
      simple_buffer_write(buf, "CONNECT", 7);
      break;
    case TRACE:
      simple_buffer_write(buf, "TRACE", 5);
      break;
    case HEAD:
      simple_buffer_write(buf, "HEAD", 4);
      break;
    case OPTIONS:
      simple_buffer_write(buf, "OPTIONS", 7);
      break;
  }

  simple_buffer(buf, " ", 1);

}



void prepared_http_request_builder_add_path(prepared_http_request_t *preq, char *path){
  simple_buffer_t *buf=preq->payload;
  simple_buffer_write_string(buf, req->path);
  simple_buffer_write(buf, " HTTP/1.1", 9);
  simple_buffer_write(buf, "\r\n",  2);
}


void prepared_http_request_builder_add_header(prepared_http_request_t *preq, char *key, char *value){
  simple_buffer_t *buf=preq->payload;
  simple_buffer_write_string(buf,current->key);
  simple_buffer_write(buf,": ",2);
  simple_buffer_write_string(buf,current->value);
  simple_buffer_write(buf,"\r\n", 2);
}


void prepared_http_request_builder_add_header_int(prepared_http_request_t *preq, char *key, int value){
  simple_buffer_t *buf=preq->payload;
  simple_buffer_write_string(buf,current->key);
  simple_buffer_write(buf,": ",2);
  simple_buffer_write_int_as_string(buf,current->value);
  simple_buffer_write(buf,"\r\n", 2);
}

void prepared_http_request_builder_add_body(prepared_http_request_t *preq, char *body, char *body_len){
  simple_buffer_t *buf=preq->payload;
  simple_buffer_write(buf,"\r\n", 2); 
  simple_buffer_write(buf,body,body_len);
}



void prepared_http_request_destroy(prepared_http_request_t *preq){
  if(preq->queue){
    request_queue_t queue=preq->queue;

    if(endpoint->queue_head==p) endpoint->queue_head=p->next;
    if(endpoint->queue_tail==p) endpoint->queue_tail=p->prev;
    if(p->prev) p->prev->next=p->next;
    if(p->next) p->next->prev=p->prev;
  }
  if(preq->conn){
    connection *conn=preq->conn;
    conn->request=NULL;
    conn->old_state=p->conn->state;
    conn->state=IDLE;
    if(conn==active_connections) {active_connections=conn->next_active;}
    if(conn->next_active) conn->next_active->prev_active=conn->prev_active;
    if(conn->prev_active) conn->prev_active->next_active=conn->next_active;
    conn->next_active=NULL;
    conn->prev_active=NULL;
  }
  if(preq->conn && preq->queue){
    preq->conn->next_idle=preq->queue->idle_list;
    preq->queue->idle_list=preq->conn;
    
  }

  preq->conn=NULL;
  destroy_simple_buffer_contents(p->payload);
  memory_pool_fixed_free(pool, preq);
}
