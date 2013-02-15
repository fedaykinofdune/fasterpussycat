#ifndef SHOGGOTH_GLOBAL_OPTIONS_H
#define SHOGGOTH_GLOBAL_OPTIONS_H
#include <stdlib.h>

struct global_options {
  size_t request_buffer_size;
  size_t read_buffer_size;
  size_t header_buffer_size;
  size_t aux_buffer_size;
  unsigned int max_endpoints;
  unsigned int max_conn_per_endpoint;
  unsigned int poll_timeout;
  unsigned int connection_timeout;
  unsigned int rw_timeout;
};

extern struct global_options opt;

#endif
