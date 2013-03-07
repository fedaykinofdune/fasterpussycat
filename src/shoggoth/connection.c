#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <zlib.h>
#include "connection.h"
#include "connection_pool.h"
#include "common/simple_buffer.h"
#include "server_endpoint.h"
#include "global_options.h"


void setup_connection(connection *conn){
  conn->read_buffer=alloc_simple_buffer(opt.read_buffer_size);
  conn->aux_buffer=alloc_simple_buffer(opt.aux_buffer_size);
  conn->fd=-1;
  conn->reused=0;
  conn->retrying=0;
  conn->state=IDLE;
  conn->srv_ctx=NULL;
  conn->srv_ssl=NULL;
  conn->response=alloc_http_response();
  conn->request=NULL;
  conn->next_idle=NULL;
  conn->next_active=NULL;
  conn->prev_active=NULL;
  conn->next_conn=NULL;
  conn->z_stream_initialized=0;
}


connection *alloc_connection(){
  connection *conn=malloc(sizeof(connection));
  setup_connection(conn);
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
  destroy_connection_zstream(conn);
}


void ready_connection_zstream(connection *conn){
   conn->z_stream.zalloc = Z_NULL;
   conn->z_stream.zfree = Z_NULL;
   conn->z_stream.opaque = Z_NULL;
   conn->z_stream.avail_in = 0;
   conn->z_stream.next_in = Z_NULL;
   inflateInit2(&conn->z_stream, 32+15);
   conn->z_stream_initialized=1;
}


void destroy_connection_zstream(connection *conn){
   if(conn->z_stream_initialized) inflateEnd(&conn->z_stream);
   conn->z_stream_initialized=0;
}

void close_connection(connection *conn){
  if(conn->fd!=-1){
    close(conn->fd);
  }
 if (conn->srv_ssl) SSL_free(conn->srv_ssl);
 if (conn->srv_ctx) SSL_CTX_free(conn->srv_ctx);
 conn->srv_ssl=NULL;
 conn->srv_ctx=NULL;
 conn->fd=-1;
  if(conn->state==IDLE) conn->old_state=NOTINIT;
  else conn->state=NOTINIT;
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
  fcntl(conn->fd, F_SETFL, fcntl(conn->fd, F_GETFL) | O_NONBLOCK);
  conn_fd[conn->index].fd=conn->fd; /* update the connection pool list of fds */
  conn_fd[conn->index].events=POLLOUT | POLLERR | POLLHUP;
  conn_fd[conn->index].revents=0;
  conn->last_rw=time(NULL);
  conn->state=CONNECTING;
  if(connect(conn->fd, ( struct sockaddr *) conn->endpoint->addr, sizeof(struct sockaddr_in)) && errno!=EINPROGRESS ) return 1;    
  if(conn->endpoint->use_ssl){
    conn->srv_ctx = SSL_CTX_new(SSLv23_client_method());
    if(!conn->srv_ctx){
      perror("wut");
      abort();
    }
    SSL_CTX_set_mode(conn->srv_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE | SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);
    conn->srv_ssl = SSL_new(conn->srv_ctx);
    if (!conn->srv_ssl) {
      SSL_CTX_free(conn->srv_ctx);
    }
    SSL_set_fd(conn->srv_ssl, conn->fd);
    SSL_set_connect_state(conn->srv_ssl);
  }
  return 0;
}

int handle_ssl_error(connection *conn, int err){
  if (err == SSL_ERROR_WANT_WRITE){
    if (conn->state==WRITING) return 0;
    conn_fd[conn->index].events=POLLOUT|POLLERR|POLLHUP;
    conn->old_state=conn->state;
    conn->state=SSL_WANT_WRITE;
    return 0;
  }
  else if (err == SSL_ERROR_WANT_READ){
    if (conn->state==READING) return 0;
    conn_fd[conn->index].events=POLLIN|POLLERR|POLLHUP;
    conn->old_state=conn->state;
    conn->state=SSL_WANT_READ;
    return 0;
  }
  return -1;
}

int read_connection_to_simple_buffer(connection *conn, simple_buffer *r_buf){
  int r_count;
  int wpos=r_buf->write_pos;
  int s=r_buf->size;
  if(s-wpos<16) {
    s << 1;
    r_buf->size=s;
    r_buf->ptr=realloc(r_buf->ptr,s);
  }
  if(conn->endpoint->use_ssl){
    r_count = SSL_read(conn->srv_ssl, r_buf->ptr+wpos , s-wpos);
  }
  else r_count=read(conn->fd, r_buf->ptr+wpos, s-wpos);
  if(r_count<-1){
    if(conn->endpoint->use_ssl){
      return handle_ssl_error(conn, SSL_get_error(conn->srv_ssl, r_count));
    }
  }
  else{
    r_buf->write_pos+=r_count;
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
  if(conn->endpoint->use_ssl) w_count = SSL_write(conn->srv_ssl, w_buf->ptr+w_buf->read_pos , w_buf->write_pos-w_buf->read_pos);
  else w_count=write(conn->fd, w_buf->ptr+w_buf->read_pos, w_buf->write_pos - w_buf->read_pos);
  if(w_count==-1){
    if(conn->endpoint->use_ssl){
      return handle_ssl_error(conn, SSL_get_error(conn->srv_ssl, w_count));
    }
  }
  else{
    w_buf->read_pos+=w_count;
  }
  return w_count;
}
