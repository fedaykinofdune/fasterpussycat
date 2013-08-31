#include <error.h>
#include <unistd.h>
#include <stdint.h>
#include <poll.h>
#include <signal.h>
#include "connection.h"
#include "connection_pool.h"
#include "global_options.h"
#include "prepared_http_request.h"
#include "http_response.h"
#include "zeromq.h"
#include "common/zeromq_common.h"

unsigned int max_total_http_connections;
unsigned int max_total_connections;
struct pollfd* conn_fd;
connection_t *non_assoc_connections=NULL;
connection_t *connection_pool;
connection_t *active_connections=NULL;
int n_hosts=0;
int zmq_fd;
int zmq_index;

void connection_pool_initialize(){
  int i;
  max_total_http_connections=opt.max_endpoints * opt.max_conn_per_endpoint;
  max_total_connections=max_total_http_connections+1;
  zmq_index=max_total_http_connections;
  conn_fd=calloc(sizeof(struct pollfd),max_total_connections);
  connection_pool=malloc(sizeof(connection) * max_total_http_connections);
  for(i=0; i<max_total_http_connections; i++){
    connection_initialize(&connection_pool[i]);
    connection_pool[i].index=i;
    conn_fd[i].events=POLLIN | POLLERR | POLLHUP;
    conn_fd[i].fd=-1;
    connection_disassociate(&connection_pool[i]);
  }
  conn_fd[zmq_index].fd=zmq_fd;
  conn_fd[zmq_index].events=POLLIN;
}

void connection_pool_disassocate(connection *conn_t){
  conn->next_idle=non_assoc_connections;
  conn->queue=NULL;
  conn->request=NULL;
  non_assoc_connections=conn;
  connection_close(conn);
}

void connection_pool_check_queues(){
  request_queue_t *queue;
  connection *conn;
  int i;
  while(n_hosts<opt.max_queues && request_queue_queue_head){
    queue=request_queue_queue_head;
    request_queue_queue_head=queue->next_queue;
    queue->next_queue=NULL;
    queue->prev_queue=NULL;
    if(queue==request_queue_queue_tail) request_queue_queue_tail=NULL;
    queue->next_working=request_queue_working_head;
    if(request_queue_working_head) request_queue_working_head->prev_working=queue;
    request_queue_working_head=queue;
    n_hosts++;
    queue->working=1;
    for(i=0; i<opt.max_conn_per_queue; i++){
       conn=non_assoc_connections;
       if(!conn){
        error(1,1,"There should be a connection left here!");
       }
       non_assoc_connections=conn->next_idle;
       connection_associate(conn, queue);
    }

  }

}



inline void connection_pool_assign_request(connection_t *idle_conn, request_queue_t *queue){
  prepared_http_request *req=queue->queue_head;
  queue->queue_head=req->next;
  if(req==queue->queue_tail) endpoint->queue_tail=NULL;
  if(req->prev) req->prev->next=req->next;
  if(req->next) req->next->prev=req->prev;
  req->next=NULL;
  req->prev=NULL;
  req->conn=idle_conn;
  idle_conn->state=idle_conn->old_state;
  idle_conn->request=req;
  if(active_connections){
    active_connections->prev_active=idle_conn;
  }
  idle_conn->next_active=active_connections;
  active_connections=idle_conn;
  connection_reset(idle_conn); 
}


void connection_pool_give_work(){
  request_queue *next;
  request_queue *working=request_queue_working_head;
  connection_t *idle_conn;
  connection_t *next_idle_conn;
  prepared_http_request *req;
  int idle;
  while(working){
    idle=0;
    next=working->next_working;
    idle_conn=working->idle_list;
    while(idle_conn){
      
      next_idle_conn=idle_conn->next_idle;
      if(!working->queue_head){
        idle++;
        idle_conn=next_idle_conn;
        continue;
      }
      working->idle_list=next_idle_conn;
      idle_conn->next_idle=NULL;
      connection_pool_assign_request(idle_conn, working);
      idle_conn=next_idle_conn;
    }
    if(idle==opt.max_conn_per_endpoint){
      request_queue_destroy(working);
    }
    working=next;
  }
}

     

void connection_pool_associate(connection_t *conn, request_queue_t *queue){
  connection_close(conn);
  conn->next_idle=queue->idle_list;
  queue->idle_list=conn;
  conn->next_conn=queue->conn_list;
  queue->conn_list=conn;
  conn->queue=queue;
}

void connection_pool_update(){
  int rc,i,do_err;
  time_t current_time=time(NULL);
  connection_t *conn, *next_conn;
  conn=active_connections;
  while(conn) {
    next_conn=conn->next_active;
    do_err=1;
    i=conn->index;
    switch(connection_pool[i].state){
      case NOTINIT:
        do_err=0;
        connection_connect(&connection_pool[i]);
        break;
      case READY:
        do_err=0;
        if(conn_fd[i].revents & (POLLERR | POLLHUP)){
          connection_connect(&connection_pool[i]);
          break;
        }
        conn_fd[i].events= POLLERR | POLLHUP | POLLOUT;
        connection_pool[i].state=WRITING;
        connection_pool[i].reused=1;
        break;
      case CONNECTING:
        if(conn_fd[i].revents & POLLOUT){
          connection_pool[i].state=WRITING;
        }
        else {
          break;
        }
      case WRITING:
        if(conn_fd[i].revents & (POLLHUP | POLLERR)){
          do_err=0;
          if(!connection_pool[i].retrying && connection_pool[i].reused){
            connection_pool[i].retrying=1;
            connect_to_endpoint(&connection_pool[i]);
          }
          else{
            connection_pool_response_err(&connection_pool[i], SHOG_ERR_CONN);
          }
        }
        else if(conn_fd[i].revents & POLLOUT){
          do_err=0;
          simple_buffer *payload=&(connection_pool[i].request.payload);
          write_connection_from_simple_buffer(&connection_pool[i],payload);
          connection_pool[i].last_rw=current_time;
          if(payload->read_pos>=payload->write_pos){
            connection_pool[i].state=READING;
            conn_fd[i].events=POLLIN | POLLERR | POLLHUP;
          }
        }
        break;
      case READING:
        rc=-1;
        if(conn_fd[i].revents & (POLLHUP | POLLERR)) {
          do_err=0;
          close_connection(&connection_pool[i]);
          rc=connection_parse(&connection_pool[i]);
        }
        else if(conn_fd[i].revents & POLLIN){
          do_err=0;
          read_connection_to_simple_buffer(&connection_pool[i],connection_pool[i].read_buffer);
          rc=connection_parse(&connection_pool[i]);
          connection_pool[i].last_rw=current_time;
        }
        if(rc==FINISHED) connection_pool_response_ok(&connection_pool[i]);
        else if(rc==INVALID) { 
          connection_pool_response_err(&connection_pool[i], SHOG_ERR_HTTP);
        }
        break;
      case SSL_WANT_READ:
        if(conn_fd[i].revents & (POLLHUP | POLLERR)) {
          do_err=0;
          connection_pool_response_err(&connection_pool[i], SHOG_ERR_CONN);
        }
        else if(conn_fd[i].revents & POLLIN){
          do_err=0;
          connection_pool[i].state=connection_pool[i].old_state;
          if(connection_pool[i].state==READING) conn_fd[i].events= POLLERR | POLLHUP | POLLIN;
          else conn_fd[i].events= POLLERR | POLLHUP | POLLOUT;
        }
        break;
      case SSL_WANT_WRITE:
        if(conn_fd[i].revents & (POLLHUP | POLLERR)) {
          do_err=0;
          connection_pool_response_err(&connection_pool[i], SHOG_ERR_CONN);
        }
        else if(conn_fd[i].revents & POLLOUT){
          do_err=0;
          connection_pool[i].state=connection_pool[i].old_state;
          if(connection_pool[i].state==READING) conn_fd[i].events= POLLERR | POLLHUP | POLLIN;
          else conn_fd[i].events= POLLERR | POLLHUP | POLLOUT;
        }
        break;
      case IDLE:
        do_err=0;
    }
    if(do_err && (current_time - connection_pool[i].last_rw > opt.rw_timeout)) connection_pool_response_err(&connection_pool[i], SHOG_ERR_TIMEOUT);
    conn=next_conn;
  }
}



void connection_pool_response_ok(connection *conn){
  http_parser_fill_response(conn->parser, conn->response);
  conn->response->status=SHOG_OK;
  conn->retrying=0;
  messaging_send_response(conn->request, conn->response);
  if(conn->parser->must_close) connection_close(conn);
  if(conn->state==READING) conn->state=READY;
  else conn->state=NOTINIT;
  prepared_http_request_destroy(conn->request);
  connection_reset(conn);
}

void connection_pool_response_err(connection *conn, uint8_t err){
  conn->response->status=err;
  conn->response->body=NULL;
  conn->response->body_len=NULL;
  conn->response->packed_headers=NULL;
  messaging_send_response(conn->request, conn->response);
  conn->endpoint->errors++;
  conn->state=NOTINIT;
  prepared_http_request(destroy(conn->request);
  close_connection(conn);
  reset_connection(conn);
}

void connection_pool_loop(){
  while(1) {  
  poll(conn_fd,max_total_connections,opt.poll_timeout);
 
  connection_pool_update();
  if(conn_fd[zmq_index].revents | POLLIN) messaging_poll_requests();
  connection_pool_check_queues();
  connection_pool_give_work();
  }
}




