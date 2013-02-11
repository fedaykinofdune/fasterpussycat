#include "connection.h"
#include "connection_pool.h"

unsigned int max_total_http_connections;
unsigned int max_total_connections;
struct pollfd* conn_fd;
connection *non_assoc_connections=NULL;
int n_hosts=0;
int zmq_fd;
int zmq_index;

void initialize_connection_pool(){
  max_total_http_connections=max_endpoints * max_conn_per_endpoint;
  max_total_connections=max_total_http_connections+1;
  zmq_index=max_total_http_connections;
  conn_fd=calloc(sizeof(pollfd),max_total_connections);
  connection_pool=malloc(sizeof(connection *) * max_http_total_connections);
  for(int i=0; i<max_total_http_connections; i++){
    connection_pool[i]=alloc_connection();
    connection_pool[i]->index=i;
    conn_fd[i].events=POLLIN | POLLERR | POLLHUP;
    conn_fd[i].fd=-1;
    dissociate_connection_from_endpoints(connection_pool[i]);
  }
  conn_fd[zmq_index].fd=zmq_fd;
  conn_fd[zmq_index].events=POLLIN:

}

void disassociate_connection_from_endpoints(connection *conn){
  non_assoc_connection->prev_idle=conn;
  conn->next_idle=non_assoc_connections;
  conn->endpoint=NULL;
  non_assoc_connections=conn;
  close_connection(conn);
}

void associate_endpoints(){
  server_endpoint *endpoint;
  connection *conn;
  while(n_hosts<max_hosts && endpoint_queue_head){
    endpoint=endpoint_queue_head;
    endpoint_queue_head=endpoint->next;
    endpoint->next=NULL;
    endpoint->prev=NULL;
    if(endpoint==endpoint_queue_tail) endpoint_queue_tail=NULL;
    endpoint->next_working=endpoint_working_head;
    endpoint_working_head->prev_working=endpoint;
    endpoint_working_head=endpoint;
    n_host++;
    for(int i=0; i<max_conn_per_host){
       conn=non_assoc_connections;
       if(!conn){
        error("There should be a connection left here!");
        abort();
       }
       non_assoc_connections=conn->next_idle;
       associate_connection_to_endpoint(conn, endpoint);
    }

  }

}




void associate_idle_connections(){
  server_endpoint *next;
  server_endpoint *working=endpoint_working_head;
  connection *idle_conn;
  connection *next_idle_conn;
  prepared_request *req;
  int idle;
  while(working){
    idle=0
    next=working->next_working;
    idle_conn=working->next_idle;
    while(idle_conn){
      next_idle_conn=idle_conn->next_idle;
      if(!working->queue_head){
        idle++;
        idle_conn=next_idle_conn;
        continue;
      }
      req=working->queue_head;
      working->queue_head=req->next;
      req->next=NULL;
      req->prev=NULL;
      conn->request=req;
      conn->retry=0;
      conn->read_buffer->read_pos=0;
      conn->read_buffer->write_pos=0;
      reset_http_response(conn->response);
      working->next_idle=next_idle_conn;
      next_idle_conn->prev_idle=NULL;
      conn->next_idle=NULL;
      conn->prev_idle=NULL;
      idle_conn=next_idle_conn;
    }
    if(idle==max_conn_per_host){
      destroy_server_endpoint(working);
    }
    working=next;
  }
}

     

void associate_connection_to_endpoint(connection *conn, server_endpoint *endpoint){
  conn->next_idle=endpoint->next_idle;
  endpoint->next_idle=conn;
  conn->next_conn=endpoint->next_conn;
  endpoint->next_conn=conn;
  conn->endpoint=endpoint;
}

void update_connections(){
  connection *conn;
  int rc;
  time_t current_time=time(NULL);
  for(int i=0; i<max_http_connections;i++){
    conn=connection_pool[i];
    if((conn->state==NOTINIT || conn->state==READY) && conn->request==NULL){
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
        conn_fd[i].revents= POLLERR | POLLHUP | POLLOUT;
        conn->state=WRITING;
        conn->reused=1;
      case CONNECTING:
        if(conn_fd[i].revents & POLLOUT){
          conn->state=WRITING;
        }
        else if((conn_fd[i].revents & (POLLERR | POLLHUP)) || current_time - conn->last_rw > connection_timeout){
          http_response_error(conn);
          break;
        }
      case WRITING:
        if(conn_fd[i].revents & POLLOUT){
          simple_buffer *payload=conn->request->payload;
          write_connection_from_simple_buffer(conn,payload);
          conn->last_rw=current_time;:
          if(payload->read_pos==payload->write_pos){
            conn->state=READING;
            conn->revents=POLLIN | POLLERR | POLLHUP;
          }
          break;
        }
        if(current_time - conn->last_rw > rw_timeout) http_response_error(conn);
        if(conn_fd[i].revents & (POLLHUP | POLLERR)){
          if(!conn->retrying && conn->reused){
            conn->retrying=1;
            connect_to_endpoint(conn);
          }
          else{
            http_response_error(conn->request);
          }
        }
        break;
      case READING:
        if(conn_fd[i].revents & POLLIN){
          read_connection_to_simple_buffer(conn,conn->read_buffer);
          conn->last_rw=current_time;
        }
        if(conn_fd[i].revents & (POLLHUP | POLLERR)) close_connection(conn);
        rc=parse_response(conn);
        if(rc==INVALID || current_time - conn->last_rw > rw_timeout) http_response_error(conn);
        if(rc==FINISHED) http_response_finished(conn);
        break;
    }
    
  }
}



void http_response_finished(connection *conn){
  send_zeromq_response_finished(conn->request,conn->response);
  destroy_prepared_http_request(conn->request);
  if(conn->state==READING) conn->state=READY;
}

void http_response_error(connection *conn){
  send_zeromq_response_error(conn->request);
  conn->endpoint->errors++;
  destroy_prepared_http_request(conn->request);
  close_connection(conn);
}

void main_connection_loop(){
  while(1) {  
  poll(conn_fd,max_total_connections,poll_timeout);
 
  update_connections();
  if(conn_fd[zmq_index].revents & POLL_IN) update_zeromq();

  associate_idle_connections();
  associate_endpoints();
  }
}




