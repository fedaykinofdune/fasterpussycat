#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <zlib.h>
#include <netinet/tcp.h>
#include "connection.h"
#include "connection_pool.h"
#include "common/simple_buffer.h"
#include "server_endpoint.h"
#include "global_options.h"


void connection_initialize(connection_t *conn){
  conn->fd=-1;
  conn->reused=0;
  conn->retrying=0;
  conn->state=IDLE;
  conn->srv_ctx=NULL;
  conn->srv_ssl=NULL;
  conn->parser=http_response_parser_alloc();
  conn->request=NULL;
  conn->next_idle=NULL;
  conn->next_active=NULL;
  conn->prev_active=NULL;
  conn->next_conn=NULL;
}


connection_t *connection_alloc(){
  connection_t *conn=malloc(sizeof(connection_t));
  connection_initialize(conn);
  return conn;
}

void connection_global_init(){
  SSL_load_error_strings ();
  SSL_library_init ();

}

void connection_reset(connection *conn){
  http_response_parser_reset(conn->parser);
}


void connection_close(connection *conn){
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



int connection_connect(connection *conn){
  close_connection(conn);
  int family=conn->endpoint.addr.ss_family;
  conn->fd=socket(family, SOCK_STREAM, 0);
  conn->reused=0;
  conn->read_buffer->write_pos=0;
  conn->read_buffer->read_pos=0;
  if(conn->fd==-1){
    perror("Could not create socket!");
  }
  fcntl(conn->fd, F_SETFL, fcntl(conn->fd, F_GETFL) | O_NONBLOCK);
  
  /* socket options */

  if(opt.tcp_nodelay) setsockopt(conn->fd,IPPROTO_TCP, TCP_NODELAY,(char *) &opt.tcp_nodelay, sizeof(int)); 
  if(opt.tcp_send_buffer) setsockopt(conn->fd, SOL_SOCKET, SO_SNDBUF, (char *) &opt.tcp_send_buffer, (int)sizeof(opt.tcp_send_buffer));
  if(opt.tcp_recv_buffer) setsockopt(conn->fd, SOL_SOCKET, SO_RCVBUF, (char *) &opt.tcp_recv_buffer, (int)sizeof(opt.tcp_recv_buffer));


  conn_fd[conn->index].fd=conn->fd; /* update the connection pool list of fds */
  conn_fd[conn->index].events=POLLOUT | POLLERR | POLLHUP;
  conn_fd[conn->index].revents=0;
  conn->last_rw=time(NULL);
  conn->state=CONNECTING;
  
  switch(family){
    case AF_INET:
      if(connect(conn->fd, ( struct sockaddr *) &(conn->endpoint.addr), sizeof(struct sockaddr_in)) && errno!=EINPROGRESS ) return 1;  
      break;
    case AF_INET6:
      if(connect(conn->fd, ( struct sockaddr *) &(conn->endpoint.addr), sizeof(struct sockaddr_in6)) && errno!=EINPROGRESS ) return 1;  
      break;
    default:
      errx(1, "Family unsupported!");
  }

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

enum http_parse_state connection_parse(connection_t *conn){
  return http_parser_parse(conn->parser, conn);

}

int connection_handle_ssl_error(connection *conn, int err){
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

int connection_read_to_simple_buffer(connection *conn, simple_buffer *r_buf){
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



void connection_add_to_request_queue(connection_t *conn, request_queue_t *queue){
  shutdown(conn->fd, SHUT_RDWR);
  conn->next_conn=queue->conn_list;
  queue->conn_list=conn;
  conn->next_idle=queue->idle_list;
  queue->idle_list=conn;
}

int connection_write_from_simple_buffer(connection_t *conn, simple_buffer_t *w_buf){
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
