#include <string.h>
#include <strings.h>
#include "address_map.h"
#include "http_request.h"
#include "prepared_http_request.h"
#include "common/simple_buffer.h"
#include "server_endpoint.h"

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
    p=p+strlen(value)+1;

    write_string_to_simple_buffer(buf,key);
    write_string_to_simple_buffer(buf,": ");
    write_string_to_simple_buffer(buf,value);
    write_string_to_simple_buffer(buf,"\r\n");
    if(!seen_host && !strcasecmp(key,"Host")) seen_host=1;
    if(!seen_cl && !strcasecmp(key,"Content-Length")) seen_cl=1;
  }
  if(!seen_host){
    write_string_to_simple_buffer(buf,"Host: ");
    concat_simple_buffer(buf,req->host);
    write_string_to_simple_buffer(buf,"\r\n");
  }

  if(!seen_cl){
    write_string_to_simple_buffer(buf,"Content-Length: ");
    write_int_to_simple_buffer(buf,req->body->write_pos);
    write_string_to_simple_buffer(buf,"\r\n");
  }
}


void write_http_request_to_simple_buffer(simple_buffer *buf, const http_request *req){
  concat_simple_buffer(buf, req->method);
  write_string_to_simple_buffer(buf, " ");
  concat_simple_buffer(buf, req->path);
  write_string_to_simple_buffer(buf, " HTTP/1.1");
  write_string_to_simple_buffer(buf, "\r\n");
  write_http_request_headers_to_simple_buffer(buf,req);
  write_string_to_simple_buffer(buf,"\r\n");
  if(req->body->size>0) concat_simple_buffer(buf,req->body);
}

/* TODO: handle ipv6 */

static void enqueue_request_step2(enum resolved_address_state state, unsigned int addr, prepared_http_request *req){
  static struct sockaddr_in addr_in;
  static server_endpoint *endpoint;
  if(state==ADDRESS_RESOLVED){
    addr_in.sin_family = AF_INET;
    addr_in.sin_port = req->port;
    unsigned char *c=(unsigned char *) &addr_in.sin_addr.s_addr;
    memcpy(&addr_in.sin_addr.s_addr, &addr, 4);
    endpoint=find_or_create_server_endpoint(&addr_in);
    if(endpoint->queue_tail){
      endpoint->queue_tail->next=req;
      req->prev=endpoint->queue_tail;
    }
    endpoint->queue_tail=req;
    if(!endpoint->queue_head) endpoint->queue_head=req;
    req->endpoint=endpoint;
  }

  /* TODO: return response immediately on fail */

}

prepared_http_request *prepare_http_request(http_request *req){
  prepared_http_request *p=alloc_prepared_http_request();
  p->options=req->options;
  p->port=req->port;
  memcpy(p->z_address,req->z_address->ptr,req->z_address->write_pos);
  p->z_address_size=req->z_address->write_pos;
  p->handle=req->handle;
  write_http_request_to_simple_buffer(p->payload, req); 
  return p;
}



void enqueue_request(http_request *req){
  char host[255];
  int s =req->host->write_pos > 254 ? 254 : req->host->write_pos;
  prepared_http_request *p=prepare_http_request(req);
  memcpy(host,req->host->ptr,s);
  host[s]=0;
  lookup_address(host,enqueue_request_step2, p); 
}
