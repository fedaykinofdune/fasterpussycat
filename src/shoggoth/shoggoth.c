static prepared_http_request_t *preq_reuse;

void shog_client_init(){
  preq_reuse=prepared_http_request_alloc();
}

void shog_enqueue(http_request_t *req, http_endpoint_t *endpoint, shog_handler_t *handler, void *data){
  prepared_http_request_reset(preq_reuse);
  promise_t *promise=promise_alloc(NORMAL, req, handler, data);  
  http_request_prepare(req, endpoint, preq_reuse);
  messaging_send_request(endpoint->tcp_endpoint, preq_reuse, promise->handle); 
}


void shog_enqueue_prepared(prepared_http_request_t *preq, http_endpoint_t *endpoint, shog_handler_t *handler, void *data){
  promise_t *promise=promise_alloc(PREPARED, preq, handler, data);  
  messaging_send_request(endpoint->tcp_endpoint, preq, promise->handle); 
}
