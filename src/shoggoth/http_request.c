#include "http_request.h"
#include "simple_buffer.h"


void add_http_header(http_request *req, const char *key, const char *value){
  http_header *header=malloc(sizeof(http_header));
  header->key=strdup(key);
  header->value=strdup(value);
  header->next=req->headers;
  req->headers=header;
}




int header_exists(http_request *req, const char *key){
  http_header *header;
  for(header=req->headers;header;header=header->next){
    if(!strncmp(key,header->key)) return 1;
  }
  return 0;
}


void write_http_request_headers_to_simple_buffer(simple_buffer *buf, const http_request *req){
  int seen_host=0;
  int seen_cl=!req->body->write_pos;
  simple_buffer *headers=req->headers;
  char *p=headers->ptr;
  char *key;
  char *value;
  char *end=headers->ptr+headers->write_pos;
  while(p<end){
    key=p;
    p=p+strlen(key)+1;
    value=p;
    p=p+strlen(value)=1;

    write_string_to_simple_buffer(buf,key);
    write_string_to_simple_buffer(buf,": ");
    write_string_to_simple_buffer(buf,value);
    write_string_to_simple_buffer("\n");
    if(!seen_host && strcasecmp(key,"Host")) seen_host=1;
    if(!seen_cl && strcasecmp(key,"Content-Length")) seen_cl=1;
  }
  if(!seen_host){
    write_string_to_simple_buffer(buf,"Host: ");
    concat_simple_buffer(buf,req->host);
    write_string_to_simple_buffer("\n");
  }

  if(!seen_cl){
    write_string_to_simple_buffer(buf,"Content-Length: ");
    write_int_to_simple_buffer(buf,req->body->write_pos);
    write_string_to_simple_buffer("\n");
  }
}


void write_http_request_to_simple_buffer(simple_buffer *buf, const http_request *req){
  concat_simple_buffer(buf, req->method);
  write_string_to_simple_buffer(buf, " ");
  concat_simple_buffer(buf, req->path);
  write_string_to_simple_buffer(buf, " ");
  write_string_to_simple_buffer(buf, "\n");
  if(!header_exists(req,"Host")) write_header_to_simple_buffer(buf,"Host", req->host);
  if(req->body->size>0 && !header_exists(req,"Content-Length")) write_header_int_to_simple_buffer(buf,"Content-Length", req->body->write_pos);
  for(http_header *cur=req->headers; cur; cur=cur->next){
    write_header_to_simple_buffer(buf,cur->key, cur->value);
  }
  write_string_to_simple_buffer(buf,"\n");
  if(req->body->size>0) concat_simple_buffer(buf,req->body);
}

void enqueue_request(http_request *req){
  prepared_http_request *p=prepare_http_request(req);
  lookup_address(req->host,enqueue_request_step2, p); 
}

/* TODO: handle ipv6 */

static void enqueue_request_step2(resolved_addess_state state, unsigned int addr, prepared_http_request *req){
  static struct sockaddr_in addr_in;
  static server_endpoint *endpoint;
  static prepared_http_request *req;
  prepared_http_request *req=(http_request *) data;
  if(state==ADDRESS_RESOLVED){
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = req->port;
    memcpy(&in_addr.sin_addr, &addr, 4);
    endpoint=find_or_create_endpoint(&addr_in);
    if(endpoint->queue_tail) endpoint->queue_tail->next=prepared;
    else endpoint->queue_head=prepared;
    endpoint->queue_tail=prepared;
    prepared->prev=NULL;
  }

  /* TODO: return response immediately on fail */

}

prepared_http_request *prepare_http_request(http_request *req){
  prepared_http_request *p=alloc_prepared_http_request();
  p->options=req->options;
  p->port=req->port;
  p->z_address=dup_simple_buffer(req->z_address);
  p->handle=req->handle;
  write_http_request_to_simple_buffer(p->payload, req); 
  return p;
}
