#include "messaging.h"


void messaging_init(){
  zeromq_init();
}


void messaging_send_request (prepared_http_request_t *preq, unsigned int handle){
  zeromq_send_response(preq,res);
}

void messaging_send_response(prepared_http_request_t *preq, http_response_t *res){
  zeromq_send_response(preq,res);
}

void messaging_poll_requests(){
  zeromq_poll_requests();
}

