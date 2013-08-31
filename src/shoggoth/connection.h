#ifndef SHOGGOTH_CONNECTION_H
#define SHOGGOTH_CONNECTION_H

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <zlib.h>

typedef struct connection connection_t;

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
  http_response_parser_t *parser;
  http_response_t response;
  simple_buffer *read_buffer;
  simple_buffer *aux_buffer;
  prepared_http_request_t *request;
  unsigned int fd;
  unsigned int index;
  time_t last_rw;
  struct sockaddr *addr;
  SSL *srv_ssl;
  SSL_CTX *srv_ctx;
  connection_t *next_conn;
  connection_t *next_idle;
  connection_t *next_active;
  connection_t *prev_active;
  tcp_endpoint_t *endpoint;
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
void ready_connection_zstream(connection *conn);
void destroy_connection_zstream(connection *conn);
#endif
