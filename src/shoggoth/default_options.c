#include "global_options.h"

struct global_options opt={
  .request_buffer_size=512,
  .read_buffer_size=200000,
  .aux_buffer_size=512,
  .header_buffer_size=1024,
  .max_endpoints=4,
  .max_conn_per_endpoint=4,
  .poll_timeout=100,
  .connection_timeout=10,
  .rw_timeout=10,
  .tcp_nodelay=1,
  .tcp_recv_buffer = 0, //16384
  .tcp_send_buffer = 0 //16384
};
