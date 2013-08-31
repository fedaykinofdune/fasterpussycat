#include "uthash.h"
#include "http_request.h"
#include "http_request_store.h"

static http_request_t *store=NULL;

void http_request_store_add(http_request_t *req){
  HASH_ADD_INT(store, handle, req);
}



http_request_t  *http_request_store_get(uint32_t handle){
  http_request_t *ret;
  HASH_FIND_INT(store, &handle, ret);
  return ret;
}


void http_request_store_del(http_request_t *req){
  HASH_DEL(store, req);
}
