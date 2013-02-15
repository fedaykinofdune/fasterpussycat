#ifndef SHOGGOTH_CONNECTION_POOL_H
#define SHOGGOTH_CONNECTION_POOL_H
#include "connection.h"

extern connection **connection_pool;
extern struct pollfd* conn_fd;

/* connection_pool.c */
void initialize_connection_pool(void);
void disassociate_connection_from_endpoints(connection *conn);
void associate_endpoints(void);
void associate_idle_connections(void);
void associate_connection_to_endpoint(connection *conn, server_endpoint *endpoint);
void update_connections(void);
void http_response_finished(connection *conn);
void http_response_error(connection *conn, uint8_t err);
void main_connection_loop(void);

#endif



