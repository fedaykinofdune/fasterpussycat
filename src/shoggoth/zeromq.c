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
  request.method=alloc_simple_buffer(0);
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


void send_zeromq_response_header(prepared_http_request *req, uint8_t stat){
  
  zmq_msg_t empty;
  zmq_msg_t z_address;
  zmq_msg_t handle;
  zmq_msg_t status;

  /* zmq address */
  
  zmq_msg_init_size(&z_address,req->z_address_size);
  memcpy(zmq_msg_data(&z_address),req->z_address, req->z_address_size);
  zmq_sendmsg(sock, &z_address, ZMQ_SNDMORE);  
  zmq_msg_close(&z_address);

  /* handle */

  zmq_msg_init_size(&handle,sizeof(int32_t));
  *((int32_t *) zmq_msg_data(&handle))=req->handle;
  zmq_sendmsg(sock, &handle, ZMQ_SNDMORE);  
  zmq_msg_close(&handle);

  /* status */

  zmq_msg_init_size(&status,sizeof(uint8_t));
  *((uint8_t *) zmq_msg_data(&status))=stat;
  zmq_sendmsg(sock, &status, (stat ? 0 : ZMQ_SNDMORE));  
  zmq_msg_close(&status);
}


void send_zeromq_response_error(prepared_http_request *req, uint8_t error_code){
  send_zeromq_response_header(req, error_code);
}



void send_zeromq_response_ok(prepared_http_request *req, http_response *res){
  
  zmq_msg_t code;
  zmq_msg_t headers;
  zmq_msg_t body;
  send_zeromq_response_header(req, Z_STATUS_OK); 

  zmq_msg_init_size(&code,sizeof(uint16_t));
  *((uint16_t *) zmq_msg_data(&code))=htons(res->code);
  zmq_sendmsg(sock, &code, ZMQ_SNDMORE);  
  zmq_msg_close(&code);

  zmq_msg_init_size(&headers,res->headers->write_pos);
  memcpy(zmq_msg_data(&headers),res->headers->ptr, res->headers->write_pos);
  zmq_sendmsg(sock, &headers, ZMQ_SNDMORE);  
  zmq_msg_close(&headers);

  zmq_msg_init_size(&body,res->body_len);
  memcpy(zmq_msg_data(&body),res->body_ptr, res->body_len);
  zmq_sendmsg(sock, &body, 0);  
  zmq_msg_close(&body);
}


void update_zeromq(){

  zmq_msg_t z_address;
  zmq_msg_t empty;
  zmq_msg_t info;
  zmq_msg_t host;
  zmq_msg_t method;
  zmq_msg_t path;
  zmq_msg_t headers;
  zmq_msg_t body;
  int size;
  size_t zmq_events_size = sizeof (uint32_t);
  uint32_t zmq_events;
  while(1){
    if (zmq_getsockopt (sock, ZMQ_EVENTS, &zmq_events, &zmq_events_size) == -1) {
      perror("something is fucked ;_;");
      abort();
    }
    if(!(zmq_events & ZMQ_POLLIN)){
      return;
    }

    zmq_msg_init (&z_address);
    zmq_msg_init (&empty);
    zmq_msg_init (&info);
    zmq_msg_init (&host);
    zmq_msg_init (&method);
    zmq_msg_init (&path);
    zmq_msg_init (&headers);
    zmq_msg_init (&body);
     
    size = zmq_recvmsg (sock, &z_address, 0);
    
    set_simple_buffer_from_ptr(request.z_address, zmq_msg_data (&z_address), size);
    
    size = zmq_recvmsg (sock, &info, 0);
    info_ptr=(packed_req_info *) zmq_msg_data(&info);
    request.handle=info_ptr->handle; /* handle remains in host byte order */
    request.port=info_ptr->port;
    request.options=ntohl(info_ptr->options);
      
    size = zmq_recvmsg (sock, &host, 0);
    set_simple_buffer_from_ptr(request.host, zmq_msg_data (&host), size);
   
    size = zmq_recvmsg (sock, &method, 0);
    set_simple_buffer_from_ptr(request.method, zmq_msg_data (&method), size);
    size = zmq_recvmsg (sock, &path, 0);
    set_simple_buffer_from_ptr(request.path, zmq_msg_data (&path), size);
    size = zmq_recvmsg (sock, &headers, 0);
    set_simple_buffer_from_ptr(request.headers, zmq_msg_data (&headers), size);
    size = zmq_recvmsg (sock, &body, 0);
    set_simple_buffer_from_ptr(request.body, zmq_msg_data (&body), size);
    enqueue_request(&request);

    zmq_msg_close (&z_address);
    zmq_msg_close (&empty);
    zmq_msg_close (&info);
    zmq_msg_close (&host);
    zmq_msg_close (&method);
    zmq_msg_close (&path);
    zmq_msg_close (&headers);
    zmq_msg_close (&body);
  }
}

