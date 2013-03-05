#include <stdint.h>

#define Z_ERR_NXDOMAIN   1
#define Z_ERR_HTTP       2
#define Z_ERR_CONNECTION 3
#define Z_ERR_TIMEOUT    4

#define Z_STATUS_OK 0

typedef struct {
  int32_t handle;
  uint32_t options;
  uint16_t port;
} packed_req_info;
