#ifndef SHOGGOTH_CONNECTION_H
#define SHOGGOTH_CONNECTION_H

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>


typedef struct connection connection;

#include "common/simple_buffer.h"
#include "prepared_http_request.h"
#include "http_response.h"
#include "server_endpoint.h"




enum connection_state { IDLE, NOTINIT, CONNECTING, READY, WRITING, READING, SSL_WANT_READ, SSL_WANT_WRITE }; 

struct connection {
  unsigned char use_ssl;
  enum connection_state state;
  enum connection_state old_state;
  unsigned char done_ssl_handshake;
  unsigned char retrying;
  unsigned char reused;
  http_response *response;
  simple_buffer *read_buffer;
  simple_buffer *aux_buffer;
  prepared_http_request *request;
  unsigned int fd;
  unsigned int index;
  time_t last_rw;
  struct sockaddr *addr;
  SSL *srv_ssl;
  SSL_CTX *srv_ctx;
  connection *next_conn;
  connection *next_idle;
  connection *next_active;
  connection *prev_active;
  server_endpoint *endpoint;

};

/* connection.c */
connection *alloc_connection();
void connection_init(void);
void reset_connection(connection *conn);
void close_connection(connection  *conn);
void setup_connection(connection  *conn);
int connect_to_endpoint(connection *conn);
int read_connection_to_simple_buffer(connection *conn, simple_buffer *r_buf);
void add_connection_to_server_endpoint(connection *conn, server_endpoint *endpoint);
int write_connection_from_simple_buffer(connection *conn, simple_buffer *w_buf);

#endif
