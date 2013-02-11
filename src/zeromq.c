#include <zeromq.h>
static void *context;
static void *socket;
static char *addr;
static char *empty
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


void send_zeromq_response_error(prepared_http_request *req){

}

void update_zeromq(){


  zmq_msg_t z_address;
  zmq_msg_t options;
  zmq_msg_t handle;
  zmq_msg_t host;
  zmq_msg_t port;
  zmq_msg_t method;
  zmq_msg_t path;
  zmq_msg_t headers;
  zmq_msg_t body;

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


    size = zmq_msg_recv (&handle, socket, 0);
    request->handle=ntohl(*((unsigned int* ) zmq_msg_data (&handle)));
   
    size = zmq_msg_recv (&host, socket, 0);
    set_simple_buffer_from_pointer(request->method, zmq_msg_data (&host), size);

    size = zmq_msg_recv (&port, socket, 0);
    request->port=*((unsigned short* ) zmq_msg_data (&port));

    size = zmq_msg_recv (&options, socket, 0);
    request->options=ntohl(*((unsigned int* ) zmq_msg_data (&options)));
   
    size = zmq_msg_recv (&method, socket, 0);
    set_simple_buffer_from_pointer(request->method, zmq_msg_data (&method), size);
   
    size = zmq_msg_recv (&path, socket, 0);
    set_simple_buffer_from_pointer(request->method, zmq_msg_data (&path), size);

    size = zmq_msg_recv (&headers, socket, 0);
    set_simple_buffer_from_pointer(request->path, zmq_msg_data (&headers), size);
    
    size = zmq_msg_recv (&body, socket, 0);
    set_simple_buffer_from_pointer(request->headers, zmq_msg_data (&body), size);
    
    enqueue_request(request);

    zmq_msg_close (&host);
    zmq_msg_close (&port);
    zmq_msg_close (&options);
    zmq_msg_close (&method);
    zmq_msg_close (&path);
    zmq_msg_close (&headers);
    zmq_msg_close (&body);
    
  }
}

