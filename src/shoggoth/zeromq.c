#include <error.h>
#include "global_options.h"
#include <zmq.h>
#include "zeromq.h"
#include "common/zeromq_common.h"
#include "http_request.h"
#include "prepared_http_request.h"
#include "sockaddr_serialize.h"


static void *context;
static void *sock;
int zmq_fd;
static simple_buffer *payload_to_send;
static packed_req_info *info_ptr;
int zeromq_init(){
  int rc;
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




void zeromq_send_request(tcp_endpoint *endpoint, prepared_http_request *preq, unsigned int handle){
  unsigned char header[SHOG_SOCKADDRSIZE+8];
  void *socket=zeromq_socket_to_use(end);
  zmq_msg_t header_msg;
  zmq_msg_t body_msg;
  int opts=htonl(preq->opts | end->use_ssl);
  sockaddr_serialize(&(endpoint->addr), header);
  memcpy(header+SHOG_SOCKADDRSIZE,&opts,4);
  memcpy(header+23,&handle,4);
  zmq_send(socket, header, SHOG_SOCKADDRSIZE+8, ZMQ_SNDMORE);
  zmq_send(socket, preq->payload->ptr, preq->payload->write_pos, 0);
}


void zeromq_send_response(prepared_http_request_t *preq, http_response_t *res){
  

  zmq_send(sock, preq->z_address,preq->z_address_size, ZMQ_SNDMORE);
  zmq_send(sock, preq->handle, sizeof(preq->handle),ZMQ_SENDMORE);
  zmq_send(sock, htonl((preq->status << 16) & preq->code), sizeof(uint32_t),ZMQ_SENDMORE);
  zmq_send(sock, res->packed_headers->ptr,res->packed_headers->write_pos,ZMQ_SENDMORE);
  zmq_send(sock, res->body, res->body_len,0);

}



void zeromq_poll_requests(){

  zmq_msg_t z_address_msg;
  zmq_msg_t payload_msg;
  zmq_msg_t header_msg;
  int size;
  size_t zmq_events_size = sizeof (uint32_t);
  uint32_t zmq_events;
  prepared_http_request_t *preq;
  while(1){

    zmq_msg_init (&z_address_msg);
    unsigned char *header;
    size = zmq_msg_recv (&z_address, sock, ZMQ_DONTWAIT);
    if(size==-1){
      zmq_msg_close(&z_address);
      if(errno!=EAGAIN) {
        perror("wut! ");
        abort();
      }
      break;
    }
    req=prepared_http_request_alloc();
    req->z_address_size=size;
    strncpy(req->z_address, zmq_msg_data (&z_address), 16);
    zmq_msg_close(&z_address); 



    size = zmq_msg_recv (&header_msg, sock, 0);
    
    header=zmq_msg_data(&header_msg);
    sockaddr_unserialize(header, &(preq->tcp_endpoint.addr));
    uint32_t opts=ntohl(*((uint32_t *) header+SHOG_SOCKADDRSIZE));
    preq->tcp_endpoint.use_ssl=opts & SHOG_USE_SSL;
    preq->opts=opts;
    preq->handle=*((uint32_t *) header+SHOG_SOCKADDRSIZE+4);
    zmq_msg_close(&header_msg);
    
    zmq_msg_init (&(preq->payload_msg));
    size=zmq_recv_msg(&(preq->payload_msg), sock, 0);
    simple_buffer_set_from_ptr(&(req->payload) , zmq_msg_data(&(req->payload_msg)), size);
    preq->payload->free=zeromq_free_prepared_payload;
    preq->payload->free_hint=&(req->payload_msg) ;
    request_queue_enqueue_request(preq);
  }
}


void zeromq_free_prepared_payload(void *throwaway, void *msg){
  zmq_msg_close(msg);
}
