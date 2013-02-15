#ifndef SHOGGOTH_CONNECTION_H
#define SHOGGOTH_CONNECTION_H

#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "simple_buffer.h"
#include "prepared_http_request.h"
#include "http_response.h"
#include "server_endpoint.h"
typedef struct connection connection;


enum connection_state { NOTINIT, CONNECTING, READY, WRITING, READING }; 

struct connection {
  unsigned char use_ssl;
  connection_state state;
  unsigned char done_ssl_handshake;
  unsigned char retry;
  unsigned char reused;
  http_response response;
  simple_buffer *read_buffer;
  simple_buffer *aux_buffer;
  prepared_request *request;
  unsigned int fd;
  unsigned int index;
  time_t lastrw;
  struct sockaddr *addr;
  SSL *sslHandle;
  SSL_CTX *sslContext;
  connection *next_conn;
  connection *next_idle;

};

/* connection.c */
connection *alloc_connection(size_t read_buffer_size, size_t write_buffer_size);
void connection_init(void);
void reset_connection(connection *conn);
void close_connection(int conn);
int connect_to_endpoint(connection *conn);
int read_connection_to_simple_buffer(connection *conn, simple_buffer *r_buf);
void add_connection_to_server_endpoint(connection *conn, server_endpoint *endpoint);
int write_connection_from_simple_buffer(connection *conn, simple_buffer *w_buf);

#endif
