#ifndef SHOGGOTH_ZEROMQ_H
#define SHOGGOTH_ZEROMQ_H

#include "common/zeromq_common.h"
#include "http_response.h"
#include "prepared_http_request.h"

extern int zmq_fd;

/* zeromq.c */
int setup_zeromq(void);
void send_zeromq_response_header(prepared_http_request *req, uint8_t status, uint16_t code);
void send_zeromq_response_error(prepared_http_request *req, uint8_t error_code);
void send_zeromq_response(prepared_http_request *req, packed_res_info *res);
void update_zeromq(void);

#endif
