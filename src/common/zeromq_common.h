#ifndef FASTERPUSSYCAT_ZEROMQ_COMMON_H
#define FASTERPUSSYCAT_ZEROMQ_COMMON_H
#include <stdint.h>

#define Z_ERR_NXDOMAIN   1
#define Z_ERR_HTTP       2
#define Z_ERR_CONNECTION 3
#define Z_ERR_TIMEOUT    4

#define Z_STATUS_OK 0

enum http_method { GET, POST, PUT, DELETE, OPTIONS, HEAD, TRACE, CONNECT };

typedef struct {
  int32_t handle;
  uint32_t options;
  uint16_t port;
  uint8_t method;
  uint32_t data_len;
  uint32_t path_offset;
  uint32_t headers_offset;
  uint32_t body_offset;
  char data[1];
} packed_req_info;



typedef struct {
  int32_t handle;
  uint8_t status;
  uint16_t code;
  uint32_t body_offset;
  uint32_t data_len;
  char data[1];
} packed_res_info;

#endif
