#include <string.h>
#include <strings.h>
#include "common/zeromq_common.h"
#include "address_map.h"
#include "http_request.h"
#include "prepared_http_request.h"
#include "common/simple_buffer.h"
#include "common/memory_pool_fixed.h"
#include "server_endpoint.h"

int last_handle_id=0;

static memory_pool_fixed_t *pool=NULL;

http_request_t *http_request_alloc(http_endpoint_t *endpoint){
  if(!pool) pool=memory_pool_fixed_t(sizeof(http_request_t,64), 
  http_request_t *req=memory_pool_fixed_alloc(pool);
  req->handle=last_handle_id++;
  req->path=NULL;
  req->headers=NULL;
  req->body=simple_buffer_alloc(64);
  req->path=simple_buffer_alloc(64);
  req->host_set=0;
  req->content_length_set=0;
  return req;
}

void *http_request_destroy(http_request_t *req){
  simple_buffer_destroy(req->path);
  simple_buffer_destroy(req->body);
  http_header_destroy_list(req->headers);
  free(req);

}



const char *http_request_get_header(const http_request_t *req, const char *key){
  return http_header_get_header(&req->headers, key);
}


void http_request_set_header(http_request_t *req, const char *key, const char *value){
  if(!strcasecmp(key,"host")){
    if(value) req->host_set=1;
    else req->host_set=0
  }

  if(!strcasecmp(key,"content_length")){
    if(value) req->content_length_set=1;
    else req->content_length_set=0
  }

  http_header_set_header(&req->headers, key, valye)

}

prepared_http_request *http_request_prepare(http_request_t *req, http_endpoint_t *endpoint, prepared_http_request_t *out){
  prepared_http_request_t *preq;
  http_header_t *header;

  if(!out) preq=prepared_http_request_alloc();
  else preq=out;
 
  /* build the prepared request */

  prepared_http_request_builder_add_method (preq, req->method); 
  prepared_http_request_builder_add_path   (preq, req->path);

  if(!req->host_set && !(req->opts & SHOG_NOHOST)){
    prepared_http_request_builder_add_header(preq, "Host", endpoint->host);
  }

  header=req->headers;
  while(header){
    if(header->value) prepared_http_request_builder_add_header(preq, header->key, header->value);
    header=header->next;
  }

  if(!req->content_length_set){
    prepared_http_request_builder_add_header_int(preq, "Content-Length", req->body->write_pos);
  }
  
  prepared_http_request_builder_add_body   (preq, req->body->ptr, req->body->write_pos);
  
  p->options=req->options;
  p->endpoint=req->endpoint->tcp_endpoint;
  p->handle=req->handle;
  return p;
}


