#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "connection.h"
#include "connection_pool.h"
#include "common/simple_buffer.h"
#include "server_endpoint.h"
#include "global_options.h"

connection *alloc_connection(){
  connection *conn=malloc(sizeof(connection));
  conn->read_buffer=alloc_simple_buffer(opt.read_buffer_size);
  conn->aux_buffer=alloc_simple_buffer(opt.aux_buffer_size);
  conn->state=0;
  conn->fd=-1;
  conn->reused=0;
  conn->retrying=0;
  conn->state=NOTINIT;
  conn->done_ssl_handshake=0;
  conn->response=alloc_http_response();
  conn->request=NULL;
  conn->next_idle=NULL;
  conn->next_conn=NULL;
  return conn;
}

void connection_init(){
  SSL_load_error_strings ();
  SSL_library_init ();

}

void reset_connection(connection *conn){
  reset_http_response(conn->response);
  reset_simple_buffer(conn->read_buffer);
  reset_simple_buffer(conn->aux_buffer);
}


void close_connection(connection *conn){
  if(conn->fd!=-1){
    close(conn->fd);
  }
  conn->fd=-1;
  conn->state=NOTINIT;
  conn_fd[conn->index].fd=-1;
  conn_fd[conn->index].events=0;
}



int connect_to_endpoint(connection *conn){
  close_connection(conn);
  conn->fd=socket(PF_INET, SOCK_STREAM, 0);
  conn->reused=0;
  conn->read_buffer->write_pos=0;
  conn->read_buffer->read_pos=0;
  if(conn->fd==-1){
    perror("Could not create socket!");
  }
  fcntl(conn->fd, F_SETFL, O_NONBLOCK);
  conn_fd[conn->index].fd=conn->fd; /* update the connection pool list of fds */
  conn_fd[conn->index].events=POLLIN | POLLOUT | POLLERR | POLLHUP;
  conn->last_rw=time(NULL);
  conn->state=CONNECTING;
  connect(conn->fd, ( struct sockaddr *) conn->endpoint->addr, sizeof(conn->endpoint->addr));    
  return 1;
}

int read_connection_to_simple_buffer(connection *conn, simple_buffer *r_buf){
  int r_count;
  if(1){
    if(r_buf->size-r_buf->write_pos<1) {
      r_buf->size=r_buf->size*2;
      r_buf->ptr=realloc(r_buf->ptr,r_buf->size);
    }
    r_count=read(conn->fd, r_buf->ptr+r_buf->write_pos, r_buf->size-r_buf->write_pos);
    if(r_count==-1){
      /* TODO handle errors */
    }
    else{
      r_buf->write_pos+=r_count;
    }
  }
  return r_count;
}



void add_connection_to_server_endpoint(connection *conn, server_endpoint *endpoint){
  shutdown(conn->fd, SHUT_RDWR);
  conn->next_conn=endpoint->conn_list;
  endpoint->conn_list=conn;
  conn->next_idle=endpoint->idle_list;
  endpoint->idle_list=conn;
}

int write_connection_from_simple_buffer(connection *conn, simple_buffer *w_buf){
  int w_count;
  if(1){
    w_count=write(conn->fd, w_buf->ptr+w_buf->read_pos, w_buf->write_pos - w_buf->read_pos);
    if(w_count==-1){
      /* TODO handle errors */
    }
    else{
      w_buf->read_pos+=w_count;
    }
  }
  return w_count;
}
