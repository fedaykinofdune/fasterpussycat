#include <error.h>
#include "global_options.h"
#include <zmq.h>
#include "zeromq.h"
#include "common/zeromq_common.h"
#include "http_request.h"
#include "prepared_http_request.h"

static void *context;
static void *sock;
static http_request request;
int zmq_fd;
static packed_req_info *info_ptr;
int setup_zeromq(){
  int rc;
  request.z_address=alloc_simple_buffer(0);
  request.path=alloc_simple_buffer(0);
  request.body=alloc_simple_buffer(0);
  request.host=alloc_simple_buffer(0);
  request.headers=alloc_simple_buffer(0);
  context = zmq_init (1);
  sock = zmq_socket (context, ZMQ_ROUTER);
  if(zmq_bind (sock, opt.bind)){
    perror("Couldn't bind zmq socket!");
    abort();
  }
  size_t s=sizeof(zmq_fd);
  zmq_getsockopt (sock, ZMQ_FD, &zmq_fd, &s);
  return zmq_fd;
}


void send_zeromq_response(prepared_http_request *req, packed_res_info *res){
  

  /* zmq address */ 
  zmq_send(sock, req->z_address,req->z_address_size, ZMQ_SNDMORE);


  /* res */

  zmq_send(sock, res, sizeof(res)+res->data_len, 0);  

}


void send_zeromq_response_error(prepared_http_request *req, uint8_t error_code){
  packed_res_info res;
  res.status=error_code;
  res.handle=req->handle;
  res.data_len=0;
  send_zeromq_response(req, &res);
}


void my_free (void *data, void *hint)
{
      //  We've allocated the buffer using malloc and
      //  at this point we deallocate it using free.
      free (data);
}

void send_zeromq_response_ok(prepared_http_request *req, http_response *res){
  simple_buffer *head=res->headers;
  uint32_t data_len=head->write_pos+res->body_len;
  uint32_t size=sizeof(packed_res_info)+data_len;
  packed_res_info *packed=malloc(size);
  packed->status=0;
  packed->code=htons(res->code);
  packed->data_len=htonl(data_len);
  packed->body_offset=htonl(head->write_pos);
  memcpy(packed->data,head->ptr, head->write_pos);
  memcpy(packed->data+head->write_pos, res->body_ptr, res->body_len);
  zmq_send(sock, req->z_address,req->z_address_size, ZMQ_SNDMORE);
  zmq_msg_t msg;
  int rc;
  rc=zmq_msg_init_data (&msg, packed, size, my_free, NULL);
  rc=zmq_msg_send(&msg, sock, 0);
  if(rc==-1){
    perror("wut2");
  }
  zmq_msg_close(&msg);

}


void update_zeromq(){

  zmq_msg_t z_address;
  zmq_msg_t msg;
  int size;
  size_t zmq_events_size = sizeof (uint32_t);
  uint32_t zmq_events;
  while(1){

    zmq_msg_init (&z_address);
     
    size = zmq_msg_recv (&z_address, sock, ZMQ_DONTWAIT);
    if(size==-1){
      zmq_msg_close(&z_address);
      if(errno!=EAGAIN) {
        perror("wut! ");
        abort();
      }
      break;
    }
    set_simple_buffer_from_ptr(request.z_address, zmq_msg_data (&z_address), size);
    
    zmq_msg_init (&msg);
    size = zmq_msg_recv (&msg, sock, 0);
    info_ptr=(packed_req_info *) zmq_msg_data(&msg);
    request.handle=info_ptr->handle; /* handle remains in host byte order */
    request.port=info_ptr->port;
    request.options=ntohl(info_ptr->options);
    request.method=info_ptr->method;
    uint32_t data_len=ntohl(info_ptr->data_len);
    uint32_t path_offset=ntohl(info_ptr->path_offset);
    uint32_t headers_offset=ntohl(info_ptr->headers_offset);
    uint32_t body_offset=ntohl(info_ptr->body_offset);
    set_simple_buffer_from_ptr(request.host, info_ptr->data, path_offset);
    set_simple_buffer_from_ptr(request.path, info_ptr->data+path_offset, headers_offset-path_offset);
    set_simple_buffer_from_ptr(request.headers, info_ptr->data+headers_offset, body_offset-headers_offset);
    set_simple_buffer_from_ptr(request.body, info_ptr->data+body_offset, data_len-body_offset);
    enqueue_request(&request);

    zmq_msg_close (&z_address);
    zmq_msg_close (&msg);
  }
}

