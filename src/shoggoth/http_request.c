#include <string.h>
#include <strings.h>
#include "common/zeromq_common.h"
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
  int kl, vl;
  char *value;
  char *end=headers->ptr+headers->write_pos;
  while(p<end){
    key=p;
    kl=strlen(key);
    p=p+kl+1;
    value=p;
    vl=strlen(value);
    p=p+vl+1;

    write_to_simple_buffer(buf,key, kl);
    write_to_simple_buffer(buf,": ", 2);
    write_to_simple_buffer(buf,value, vl);
    write_to_simple_buffer(buf,"\r\n", 2);
    if(!seen_host && !strcasecmp(key,"Host")) seen_host=1;
    if(!seen_cl && !strcasecmp(key,"Content-Length")) seen_cl=1;
  }
  if(!seen_host){
    write_to_simple_buffer(buf,"Host: ", 6);
    concat_simple_buffer(buf,req->host);
    write_to_simple_buffer(buf,"\r\n", 2);
  }

  if(!seen_cl){
    write_to_simple_buffer(buf,"Content-Length: ", 16);
    write_int_to_simple_buffer(buf,req->body->write_pos);
    write_to_simple_buffer(buf,"\r\n", 2);
  }
}


void write_http_request_to_simple_buffer(simple_buffer *buf, const http_request *req){
  switch(req->method){
    case GET:
      write_to_simple_buffer(buf, "GET", 3);
      break;
    case POST:
      write_to_simple_buffer(buf, "POST", 4);
      break;
    case PUT:
      write_to_simple_buffer(buf, "PUT", 3);
      break;
    case DELETE:
      write_to_simple_buffer(buf, "DELETE", 6);
      break;
    case CONNECT:
      write_to_simple_buffer(buf, "CONNECT", 7);
      break;
    case TRACE:
      write_to_simple_buffer(buf, "TRACE", 5);
      break;
    case HEAD:
      write_to_simple_buffer(buf, "HEAD", 4);
      break;
    case OPTIONS:
      write_to_simple_buffer(buf, "OPTIONS", 7);
      break;
  }
  write_to_simple_buffer(buf, " ", 1);
  concat_simple_buffer(buf, req->path);
  write_to_simple_buffer(buf, " HTTP/1.1", 9);
  write_to_simple_buffer(buf, "\r\n",  2);
  write_http_request_headers_to_simple_buffer(buf,req);
  write_to_simple_buffer(buf,"\r\n", 2); 
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
    endpoint->use_ssl=req->options & OPT_USE_SSL;
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
