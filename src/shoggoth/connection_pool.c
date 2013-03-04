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
connection *non_assoc_connections=NULL;
connection **connection_pool;
int n_hosts=0;
int zmq_fd;
int zmq_index;

void initialize_connection_pool(){
  int i;
  max_total_http_connections=opt.max_endpoints * opt.max_conn_per_endpoint;
  max_total_connections=max_total_http_connections+1;
  zmq_index=max_total_http_connections;
  conn_fd=calloc(sizeof(struct pollfd),max_total_connections);
  connection_pool=malloc(sizeof(connection *) * max_total_http_connections);
  for(i=0; i<max_total_http_connections; i++){
    connection_pool[i]=alloc_connection();
    connection_pool[i]->index=i;
    conn_fd[i].events=POLLIN | POLLERR | POLLHUP;
    conn_fd[i].fd=-1;
    disassociate_connection_from_endpoints(connection_pool[i]);
  }
  conn_fd[zmq_index].fd=zmq_fd;
  conn_fd[zmq_index].events=POLLIN;
}

void disassociate_connection_from_endpoints(connection *conn){
  conn->next_idle=non_assoc_connections;
  conn->endpoint=NULL;
  conn->request=NULL;
  non_assoc_connections=conn;
  close_connection(conn);
}

void associate_endpoints(){
  server_endpoint *endpoint;
  connection *conn;
  int i;
  while(n_hosts<opt.max_endpoints && endpoint_queue_head){
    endpoint=endpoint_queue_head;
    endpoint_queue_head=endpoint->next;
    endpoint->next=NULL;
    endpoint->prev=NULL;
    if(endpoint==endpoint_queue_tail) endpoint_queue_tail=NULL;
    endpoint->next_working=endpoint_working_head;
    if(endpoint_working_head) endpoint_working_head->prev_working=endpoint;
    endpoint_working_head=endpoint;
    n_hosts++;
    for(i=0; i<opt.max_conn_per_endpoint; i++){
       conn=non_assoc_connections;
       if(!conn){
        error(1,1,"There should be a connection left here!");
       }
       non_assoc_connections=conn->next_idle;
       associate_connection_to_endpoint(conn, endpoint);
    }

  }

}



inline void associate_idle_connection_with_next_endpoint_request(connection *idle_conn, server_endpoint *endpoint){
  prepared_http_request *req=endpoint->queue_head;
  endpoint->queue_head=req->next;
  if(req==endpoint->queue_tail) endpoint->queue_tail=NULL;
  if(req->prev) req->prev->next=req->next;
  if(req->next) req->next->prev=req->prev;
  req->next=NULL;
  req->prev=NULL;
  req->conn=idle_conn;
  idle_conn->request=req;
  reset_connection(idle_conn); 
}


void associate_idle_connections(){
  server_endpoint *next;
  server_endpoint *working=endpoint_working_head;
  connection *idle_conn;
  connection *next_idle_conn;
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
      associate_idle_connection_with_next_endpoint_request(idle_conn, working);
      idle_conn=next_idle_conn;
    }
    if(idle==opt.max_conn_per_endpoint){
      destroy_server_endpoint(working);
    }
    working=next;
  }
}

     

void associate_connection_to_endpoint(connection *conn, server_endpoint *endpoint){
  close_connection(conn);
  conn->next_idle=endpoint->idle_list;
  endpoint->idle_list=conn;
  conn->next_conn=endpoint->conn_list;
  endpoint->conn_list=conn;
  conn->endpoint=endpoint;
}

void update_connections(){
  connection *conn;
  int rc,i;
  time_t current_time=time(NULL);
  for(i=0; i<max_total_http_connections;i++){
    conn=connection_pool[i];
    if(conn->endpoint==NULL || conn->request==NULL){
      continue;
    }

    switch(conn->state){
      case NOTINIT:
        connect_to_endpoint(conn);
        break;
      case READY:
        if(conn_fd[i].revents & (POLLERR | POLLHUP)){
          connect_to_endpoint(conn);
          break;
        }
        conn_fd[i].events= POLLERR | POLLHUP | POLLOUT;
        conn->state=WRITING;
        conn->reused=1;
      case CONNECTING:
        if(conn_fd[i].revents & POLLOUT){
          conn->state=WRITING;
        }
        else if((conn_fd[i].revents & (POLLERR | POLLHUP)) || current_time - conn->last_rw > opt.connection_timeout){
          http_response_error(conn,(conn_fd[i].revents & (POLLERR | POLLHUP)) ? Z_ERR_CONNECTION : Z_ERR_TIMEOUT);
          break;
        }
      case WRITING:
        if(conn_fd[i].revents & (POLLHUP | POLLERR)){
          if(!conn->retrying && conn->reused){
            conn->retrying=1;
            connect_to_endpoint(conn);
          }
          else{
            http_response_error(conn, Z_ERR_CONNECTION);
          }
          break;
        }
        if(conn_fd[i].revents & POLLOUT){
          simple_buffer *payload=conn->request->payload;
          write_connection_from_simple_buffer(conn,payload);
          conn->last_rw=current_time;
          if(payload->read_pos>=payload->write_pos){
            conn->state=READING;
            conn_fd[i].events=POLLIN | POLLERR | POLLHUP;
          }
          break;
        }
        if(current_time - conn->last_rw > opt.rw_timeout) http_response_error(conn,Z_ERR_TIMEOUT);
        break;
      case READING:
        if(conn_fd[i].revents & POLLIN){
          read_connection_to_simple_buffer(conn,conn->read_buffer);
          conn->last_rw=current_time;
        }
        if(conn_fd[i].revents & (POLLHUP | POLLERR)) close_connection(conn);
        rc=parse_connection_response(conn);
        if(rc==FINISHED) http_response_finished(conn);
        if(rc==INVALID || current_time - conn->last_rw > opt.rw_timeout) http_response_error(conn, rc==INVALID ? Z_ERR_HTTP : Z_ERR_TIMEOUT);
        break;
    }
    conn_fd[i].revents=0;
    
  }
}



void http_response_finished(connection *conn){
  send_zeromq_response_ok(conn->request,conn->response);
  destroy_prepared_http_request(conn->request);
  if(conn->response->must_close) close_connection(conn);
  if(conn->state==READING) conn->state=READY;
  else conn->state=NOTINIT;
  reset_connection(conn);
}

void http_response_error(connection *conn, uint8_t err){
  send_zeromq_response_error(conn->request, err);
  conn->endpoint->errors++;
  destroy_prepared_http_request(conn->request);
  conn->state=NOTINIT;
  close_connection(conn);
  reset_connection(conn);
}

void main_connection_loop(){
  while(1) {  
  // printf("loop %lld\n", (long long) time(NULL));
  poll(conn_fd,max_total_connections,opt.poll_timeout);
 
  update_connections();
  if(conn_fd[zmq_index].revents | POLLIN) update_zeromq();

  associate_idle_connections();
  associate_endpoints();
  }
}




