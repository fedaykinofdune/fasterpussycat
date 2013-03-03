#include <errno.h>
#include <zmq.h>
#include <string.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#include "../common/simple_buffer.h"
#include "../common/zeromq_common.h"
#include "shoggoth_api.h"

#define MAX_SOCKETS 32
#define POLL_TIMEOUT 1000

static http_request *request=NULL;

static zmq_pollitem_t pollitems[MAX_SOCKETS];
static void *sockets[MAX_SOCKETS];
static int n_pollitems=0;
static int next_socket;
static void *context;
static const struct luaL_reg api [] = {
        {"enqueue_http_request", l_enqueue_http_request},
        {"poll", l_poll},
        {"connect_endpoint", l_connect_endpoint},
        {NULL, NULL}  /* sentinel */

};




void register_shoggoth_api(lua_State *L){
  luaL_register(L, "shoggoth", api);
  request=malloc(sizeof(http_request));
  request->host=alloc_simple_buffer(32);
  request->method=alloc_simple_buffer(8);
  request->path=alloc_simple_buffer(128);
  request->headers=alloc_simple_buffer(128);
  request->body=alloc_simple_buffer(1024);
  context=zmq_init (1);
}

int l_connect_endpoint(lua_State *L){
  const char *endpoint=lua_tostring(L,1);
  void *sock;
  sock = zmq_socket (context, ZMQ_DEALER);
  if(zmq_connect (sock, endpoint)){
     printf(":(((( %s\n",strerror(errno));
      luaL_error(L, strerror(errno));
  }
  sockets[n_pollitems]=sock;
  pollitems[n_pollitems].socket=sock;
  pollitems[n_pollitems].events=ZMQ_POLLIN;
  n_pollitems++;
  return 0;
}

int l_poll(lua_State *L){
  lua_newtable(L);
  zmq_poll(pollitems, n_pollitems, POLL_TIMEOUT);
  int i;
  for(i=0;i<n_pollitems;i++){
    if(pollitems[i].revents & ZMQ_POLLIN){
      l_raw_poll(L,pollitems[i].socket);
    }
  }
  return 1;

}



inline void *get_zmq_socket_to_use(http_request *req){
  return sockets[(next_socket++ % n_pollitems)];
}

void l_raw_poll(lua_State *L, void *sock){

  zmq_msg_t status;
  zmq_msg_t handle;
  zmq_msg_t code;
  zmq_msg_t headers;
  zmq_msg_t body;
  zmq_msg_t error;
  int size;
  int result_table=lua_gettop(L);
  luaL_checktype(L, result_table, LUA_TTABLE);
  int index=lua_objlen(L, result_table);
  static http_request *request=NULL;
  size_t zmq_events_size = sizeof (uint32_t);
  uint32_t zmq_events;
  int handle_i;
  int status_i;
  int error_i;
  int code_i;
  while(1){
    index++;
    if (zmq_getsockopt (sock, ZMQ_EVENTS, &zmq_events, &zmq_events_size) == -1) {
      perror("something is fucked ;_;");
      abort();
    }
    if(!(zmq_events & ZMQ_POLLIN)){
      return;
    }
    
    lua_newtable(L);
    zmq_msg_init(&handle);
    
    size=zmq_recvmsg (&handle, sock, 0);
    handle_i=*((int32_t* ) zmq_msg_data (&handle)); /* handle remains in host byte order */
    lua_rawgeti( L, LUA_REGISTRYINDEX, handle_i );
    lua_setfield(L, -2, "request");
    luaL_unref(L, LUA_REGISTRYINDEX, handle_i );
    zmq_msg_close(&handle);
    
    size=zmq_recvmsg (&status, sock, 0);
    status_i=*((uint8_t*) zmq_msg_data (&status));
    lua_pushinteger(L,status_i);
    lua_setfield(L, -2, "status");
    zmq_msg_close(&status);

    if(status_i==Z_STATUS_ERR){
      size=zmq_recvmsg (&error, sock, 0);
      error_i=*((uint8_t*) zmq_msg_data (&error));
      lua_pushinteger(L, error_i);
      lua_setfield(L, -2, "error");
      zmq_msg_close(&error);
      lua_rawseti(L, result_table, index);
      continue;
    }

    zmq_msg_init(&code);
    size=zmq_recvmsg (&code, sock, 0);
    code_i=*((uint16_t*) zmq_msg_data (&code));
    lua_pushinteger(L, ntohs(code_i));
    lua_setfield(L, -2, "code");
    zmq_msg_close(&code);

    size=zmq_recvmsg (&headers, sock, 0);
    char *ptr=zmq_msg_data(&headers);
    char *end=ptr+size;
    char *key;
    char *value;
    lua_newtable(L);
    while(ptr<end){
      key=ptr;
      ptr=ptr+strlen(key)+1;
      value=ptr;
      ptr=ptr+strlen(value)+1;
      lua_pushstring(L,value);
      lua_setfield(L, -2, key);
    }
    lua_setfield(L, -2, "headers");
    zmq_msg_close(&headers);

    size=zmq_recvmsg (&body, sock, 0);
    lua_pushstring(L, zmq_msg_data(&body));
    lua_setfield(L, -2, "body");
    zmq_msg_close(&body);

    lua_rawseti(L, result_table, index);
  }
}



inline void raw_send_http_request(void *socket, http_request *req){
  zmq_send(socket, 0,0 ,ZMQ_SNDMORE);
  zmq_send(socket, &req->handle,sizeof(uint32_t),ZMQ_SNDMORE);
  zmq_send(socket, req->host->ptr, req->host->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, &req->port, sizeof(uint16_t),ZMQ_SNDMORE);
  zmq_send(socket, &req->options, sizeof(uint32_t),ZMQ_SNDMORE);
  zmq_send(socket, req->method->ptr, req->method->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, req->path->ptr, req->path->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, req->headers->ptr, req->headers->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, req->body->ptr, req->body->write_pos, 0);
}




inline void send_http_request(lua_State *L, http_request *req){
  if (!n_pollitems) {
    luaL_error(L, "No endpoints available!");
  }
  void *socket=get_zmq_socket_to_use(req);
  raw_send_http_request(socket, req);

}

int l_enqueue_http_request(lua_State *L){
  http_request *req=request;
  lua_pushvalue (L, 1);
  req->handle=luaL_ref(L, LUA_REGISTRYINDEX );
  
  
  reset_simple_buffer(req->host);
  lua_getfield (L, 1, "host");
  write_string_to_simple_buffer(req->host, lua_tostring(L, -1));
  lua_pop(L,1);
  
  reset_simple_buffer(req->method);
  lua_getfield (L, 1, "method");
  write_string_to_simple_buffer(req->method, lua_tostring(L, -1));
  lua_pop(L,1);
  
  reset_simple_buffer(req->path);
  lua_getfield (L, 1, "path");
  write_string_to_simple_buffer(req->path, lua_tostring(L, -1));
  lua_pop(L,1);
  
  reset_simple_buffer(req->body);
  lua_getfield (L, 1, "body");
  write_string_to_simple_buffer(req->body, lua_tostring(L, -1));
  lua_pop(L,1);

  lua_getfield (L, 1, "port");
  request->port=htons(lua_tointeger (L, -1));
  lua_pop(L,1);

  lua_getfield (L, 1, "options");
  request->options=htons(lua_tointeger (L, -1));
  lua_pop(L,1);


  reset_simple_buffer(req->headers);
  lua_getfield (L, 1, "headers");
  lua_pushnil(L);
  int j;
  while (lua_next(L, -2) != 0) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    write_packed_string_to_simple_buffer2(request->headers, lua_tostring(L,-2));
    write_packed_string_to_simple_buffer2(request->headers, lua_tostring(L,-1));
    lua_pop(L, 1);
  }
  lua_pop(L,1);

  lua_pushvalue(L,1);
  request->handle=luaL_ref(L,LUA_REGISTRYINDEX);
  send_http_request(L, request);
  return 0;
}




