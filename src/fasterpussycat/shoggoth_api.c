#include <errno.h>
#include <zmq.h>
#include <string.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#include "common/simple_buffer.h"
#include "shoggoth_api.h"

#define MAX_SOCKETS 32
#define POLL_TIMEOUT 100000

static http_request request;

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
  request.host=alloc_simple_buffer(32);
  request.method=alloc_simple_buffer(8);
  request.path=alloc_simple_buffer(128);
  request.headers=alloc_simple_buffer(128);
  request.body=alloc_simple_buffer(1024);
  context=zmq_init (1);
}

int l_connect_endpoint(lua_State *L){
  const char *endpoint=lua_tostring(L,1);
  void *sock;
  sock = zmq_socket (context, ZMQ_DEALER);
  if(zmq_connect (sock, endpoint)){
      luaL_error(L, strerror(errno));
  }
  sockets[n_pollitems]=sock;
  pollitems[n_pollitems].socket=sock;
  pollitems[n_pollitems].fd=0;
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
  return sockets[(++next_socket % n_pollitems)];
}

void l_raw_poll(lua_State *L, void *sock){

  zmq_msg_t status;
  zmq_msg_t handle;
  zmq_msg_t code;
  zmq_msg_t headers;
  zmq_msg_t empty;
  zmq_msg_t body;
  zmq_msg_t error;
  int64_t more;
  size_t more_size=sizeof(more);
  int size;
  int result_table=lua_gettop(L);
  luaL_checktype(L, result_table, LUA_TTABLE);
  int index=lua_objlen(L, result_table);
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
    
    size=zmq_recvmsg (sock, &handle, 0);
    if(size==-1){
       perror("wut");
    }
    handle_i=*((int32_t* ) zmq_msg_data (&handle)); /* handle remains in host byte order */
    lua_rawgeti( L, LUA_REGISTRYINDEX, handle_i );
    lua_setfield(L, -2, "request");
    luaL_unref(L, LUA_REGISTRYINDEX, handle_i );
    zmq_msg_close(&handle);
    
    zmq_msg_init(&status);
    size=zmq_recvmsg (sock, &status, 0);
    
    status_i=*((uint8_t*) zmq_msg_data (&status));
    lua_pushinteger(L,status_i);
    lua_setfield(L, -2, "status");
    zmq_msg_close(&status);

    if(status_i){
      lua_rawseti(L, result_table, index);
      zmq_getsockopt (sock, ZMQ_RCVMORE, &more, &more_size);
      if(more){
        abort();
      }
      continue;
    }

    zmq_msg_init(&code);
    size=zmq_recvmsg (sock, &code, 0);
    code_i=*((uint16_t*) zmq_msg_data (&code));
    lua_pushinteger(L, ntohs(code_i));
    lua_setfield(L, -2, "code");
    zmq_msg_close(&code);

    zmq_msg_init(&headers);
    size=zmq_recvmsg (sock, &headers, 0);
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

    zmq_msg_init(&body);
    size=zmq_recvmsg (sock, &body, 0);
    lua_pushlstring(L, zmq_msg_data(&body),size);
    lua_setfield(L, -2, "body");
    zmq_msg_close(&body);

    lua_rawseti(L, result_table, index);
  }
}



inline void raw_send_http_request(void *socket){
  zmq_send(socket, &request.info,sizeof(request.info),ZMQ_SNDMORE);
  zmq_send(socket, request.host->ptr, request.host->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, request.method->ptr, request.method->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, request.path->ptr, request.path->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, request.headers->ptr, request.headers->write_pos, ZMQ_SNDMORE);
  zmq_send(socket, request.body->ptr, request.body->write_pos, 0);
}




inline void send_http_request(lua_State *L){
  if (!n_pollitems) {
    luaL_error(L, "No endpoints available!");
  }
  void *socket=get_zmq_socket_to_use(&request);
  raw_send_http_request(socket);

}

int l_enqueue_http_request(lua_State *L){
  lua_pushvalue (L, 1);
  request.info.handle=luaL_ref(L, LUA_REGISTRYINDEX );
  
  
  reset_simple_buffer(request.host);
  lua_getfield (L, 1, "host");
  write_string_to_simple_buffer(request.host, lua_tostring(L, -1));
  lua_pop(L,1);
  
  reset_simple_buffer(request.method);
  lua_getfield (L, 1, "method");
  write_string_to_simple_buffer(request.method, lua_tostring(L, -1));
  lua_pop(L,1);
  
  reset_simple_buffer(request.path);
  lua_getfield (L, 1, "path");
  write_string_to_simple_buffer(request.path, lua_tostring(L, -1));
  lua_pop(L,1);
  
  reset_simple_buffer(request.body);
  lua_getfield (L, 1, "body");
  write_string_to_simple_buffer(request.body, lua_tostring(L, -1));
  lua_pop(L,1);

  lua_getfield (L, 1, "port");
  request.info.port=htons(lua_tointeger (L, -1));
  lua_pop(L,1);

  lua_getfield (L, 1, "options");
  request.info.options=htons(lua_tointeger (L, -1));
  lua_pop(L,1);


  reset_simple_buffer(request.headers);
  lua_getfield (L, 1, "headers");
  lua_pushnil(L);
  int j;
  while (lua_next(L, -2) != 0) {
    /* uses 'key' (at index -2) and 'value' (at index -1) */
    write_packed_string_to_simple_buffer2(request.headers, lua_tostring(L,-2));
    write_packed_string_to_simple_buffer2(request.headers, lua_tostring(L,-1));
    lua_pop(L, 1);
  }
  lua_pop(L,1);

  send_http_request(L);
  return 0;
}




