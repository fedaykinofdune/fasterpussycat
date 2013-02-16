#ifndef SHOGGOTH_ZEROMQ_H
#define SHOGGOTH_ZEROMQ_H

#include "http_response.h"
#include "prepared_http_request.h"

/* zeromq.c */
int setup_zeromq(void);
void send_zeromq_response_header(prepared_http_request *req, uint8_t status);
void send_zeromq_response_error(prepared_http_request *req, uint8_t error_code);
void send_zeromq_response_ok(prepared_http_request *req, http_response *res);
void update_zeromq(void);

#endif
