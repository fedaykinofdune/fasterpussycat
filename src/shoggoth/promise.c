static int top_handle=0;
static promise_t *promises=NULL;

promise_t *promise_make(enum shoggoth_request_type req_type, void *request, shog_handler_t *handler, void *data){
  promise_t *promise=malloc(sizeof(promise_t));
  promise->handle=top_handle;
  promise->handler=handler;
  promise->data=data;
  top_handle++;
  switch(req_type){
    case NORMAL:
      promise->req=(http_request *) request;
    case PREPARED:
      promise->preq=(prepared_http_request *) request;
    default:
      errx(1,"Unknown type");
  }
  promise->request_type=req;
  HASH_ADD_INT(promises, handle, promise ); 
  return promise;
}

void promise_destroy(promise_t *promise){
  free(promise);
  HASH_DEL(promises, promise);
}



void promise_retrieve(unsigned int handle){
  promise_t *promise
  HASH_FIND_INT(promises, &handle, promise);
}

void promise_keep(promise_t *promise, http_response_t *response){
  shog_http_event_t event;
  switch(promise->req_type){
    case NORMAL:
      event.req=promise->req;
    case PREPARED:
      event.preq=promise->preq;
    default:
      errx(1,"Unknown type");
  }
  event.req_type=promise.req_type;
  event.res=response;
  event.status=response.status;
  promise->handler(&event, promise->data);
  promise_destroy(promise);
}
