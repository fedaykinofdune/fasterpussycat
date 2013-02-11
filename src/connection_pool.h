#include "connection.h"

extern connection **connection_pool;
struct pollfd* conn_fd;
extern unsigned int max_total_connections;
extern unsigned int max_hosts;
extern unsigned int max_per_host;



