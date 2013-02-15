#include <zmq.h>
#include "zeromq.h"
#include "../common/zeromq_common.h"
#include "http_request.h"
#include "prepared_http_request.h"
static void *context;
static void *socket;
static char *addr;
static char *empty;
static http_request *request;

int setup_zeromq(){
  request=malloc(sizeof(http_request);
  context = zmq_ctx_new ();
  socket = zmq_socket (context, ZMQ_ROUTER);
  zmq_bind (worker, "ipc://client.ipc");
  int zeromq_fd;
  int s=sizeof(int);
  zmq_getsockopt (socket, ZMQ_FD, &zeromq_fd, &s);
  return zeromq_fd;
}


void send_zeromq_response_header(prepared_http_request *req, uint8_t status){
  zmq_msg_t empty;
  zmq_msg_t z_address;
  zmq_msg_t handle;
  zmq_msg_t status;

  /* zmq address */
  
  zmq_msg_init_size(&z_address,req->z_address->size);
  memcpy(zmq_msg_data(&z_address),req->z_address->ptr, req->z_address->size);
  zmq_msg_send(socket, &z_address, ZMQ_SNDMORE);  
  zmq_msg_close(&z_address);

  /* empty msg delimiter */

  zmq_msg_init_size(&empty,0);
  zmq_msg_send(socket, &empty, ZMQ_SNDMORE);  
  zmq_msg_close(&empty);

  /* handle */

  zmq_msg_init_size(&handle,sizeof(uint32_t));
  *((uint32_t *) zmq_msg_data(&handle))=req->handle;
  zmq_msg_send(socket, &handle, ZMQ_SNDMORE);  
  zmq_msg_close(&handle);

  /* status */

  zmq_msg_init_size(&status,sizeof(uint8_t));
  *((uint8_t) zmq_msg_data(&handle))=status;
  zmq_msg_send(socket, &handle, ZMQ_SNDMORE);  

}


void send_zeromq_response_error(prepared_http_request *req, uint8_t error_code){
  
  zmq_msg_t err;

  send_zeromq_response_header(req, Z_STATUS_ERR);  

  zmq_msg_init_size(&err,sizeof(uint8_t));
  *((uint32_t *) zmq_msg_data(&err))=error_code;
  zmq_msg_send(socket, &err, 0);  
  zmq_msg_close(&err);
}



void send_zeromq_response_ok(prepared_http_request *req, http_response *res){
  
  zmq_msg_t code;
  zmq_msg_t headers;
  zmq_msg_t body;

  send_zeromq_response_header(req, Z_STATUS_OK); 

  zmq_msg_init_size(&code,sizeof(uint16_t));
  *((uint16_t *) zmq_msg_data(&code))=htons(res->code);
  zmq_msg_send(socket, &code, ZMQ_SNDMORE);  
  zmq_msg_close(&code);

  zmq_msg_init_size(&headers,sizeof(res->headers->write_pos));
  memcpy(zmq_msg_data(&headers),res->headers->ptr, res->headers->write_pos);
  zmq_msg_send(socket, &headers, ZMQ_SNDMORE);  
  zmq_msg_close(&headers);

  zmq_msg_init_size(&body,sizeof(res->body_len));
  memcpy(zmq_msg_data(&headers),res->body_ptr, res->body->write_pos);
  zmq_msg_send(socket, &body, 0);  
  zmq_msg_close(&body);
}


void update_zeromq(){


  zmq_msg_t z_address;
  zmq_msg_t empty;
  zmq_msg_t options;
  zmq_msg_t handle;
  zmq_msg_t host;
  zmq_msg_t port;
  zmq_msg_t method;
  zmq_msg_t path;
  zmq_msg_t headers;
  zmq_msg_t body;
  static http_request request=NULL; 
  if(!request) request=malloc(sizeof(http_request)); 
  int size;
  size_t zmq_events_size = sizeof (uint32_t);
  uint32_t zmq_events;
  while(1){
    if (zmq_getsockopt (socket, ZMQ_EVENTS, &zmq_events, &zmq_events_size) == -1) {
      perror("something is fucked ;_;");
      abort();
    }
    if(!(zmq_events & ZMQ_POLLIN)){
      return;
    }


    zmq_msg_init (&z_address);
    zmq_msg_init (&empty);
    zmq_msg_init (&handle);
    zmq_msg_init (&host);
    zmq_msg_init (&options);
    zmq_msg_init (&port);
    zmq_msg_init (&options);
    zmq_msg_init (&method);
    zmq_msg_init (&path);
    zmq_msg_init (&headers);
    zmq_msg_init (&body);
     
    size = zmq_msg_recv (&z_address, socket, 0);
    set_simple_buffer_from_pointer(request->z_address, zmq_msg_data (&z_address), size);

    size = zmq_msg_recv (&empty, socket, 0);
    if(size!=0){
      error("Can't find empty delimiter in zeromq msg!");
      abort();
    }
    size = zmq_msg_recv (&handle, socket, 0);

    request->handle=*((uint32_t* ) zmq_msg_data (&handle)); /* handle remains in host byte order */
   
    size = zmq_msg_recv (&host, socket, 0);
    set_simple_buffer_from_pointer(request->method, zmq_msg_data (&host), size);

    size = zmq_msg_recv (&port, socket, 0);
    request->port=*((uint16_t* ) zmq_msg_data (&port)); /* port remains in network byte order */

    size = zmq_msg_recv (&options, socket, 0);
    request->options=ntohl(*((uint32_t* ) zmq_msg_data (&options)));
   
    size = zmq_msg_recv (&method, socket, 0);
    set_simple_buffer_from_pointer(request->method, zmq_msg_data (&method), size);
   
    size = zmq_msg_recv (&path, socket, 0);
    set_simple_buffer_from_pointer(request->method, zmq_msg_data (&path), size);

    size = zmq_msg_recv (&headers, socket, 0);
    set_simple_buffer_from_pointer(request->path, zmq_msg_data (&headers), size);
    
    size = zmq_msg_recv (&body, socket, 0);
    set_simple_buffer_from_pointer(request->headers, zmq_msg_data (&body), size);
    
    enqueue_request(request);

    zmq_msg_close (&z_address);
    zmq_msg_close (&empty);
    zmq_msg_close (&host);
    zmq_msg_close (&port);
    zmq_msg_close (&options);
    zmq_msg_close (&method);
    zmq_msg_close (&path);
    zmq_msg_close (&headers);
    zmq_msg_close (&body);
    
  }
}

