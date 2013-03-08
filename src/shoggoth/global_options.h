#ifndef SHOGGOTH_GLOBAL_OPTIONS_H
#define SHOGGOTH_GLOBAL_OPTIONS_H
#include <stdlib.h>

struct global_options {
  size_t request_buffer_size;
  size_t read_buffer_size;
  size_t header_buffer_size;
  size_t aux_buffer_size;
  char *bind;
  unsigned int tcp_nodelay;
  unsigned int max_endpoints;
  unsigned int max_conn_per_endpoint;
  unsigned int poll_timeout;
  unsigned int connection_timeout;
  unsigned int rw_timeout;
  unsigned int tcp_recv_buffer;
  unsigned int tcp_send_buffer;
};

extern struct global_options opt;

#endif
