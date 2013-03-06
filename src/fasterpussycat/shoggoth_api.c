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

  zmq_msg_t msg;
  packed_res_info *res;
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
    zmq_msg_init(&msg);
    size=zmq_msg_recv(&msg,sock,0);
    res=(packed_res_info *) zmq_msg_data(&msg);
    if(size==-1){
       perror("wut");
    }
    handle_i=res->handle; /* handle remains in host byte order */
    lua_rawgeti( L, LUA_REGISTRYINDEX, handle_i );
    lua_setfield(L, -2, "request");
    luaL_unref(L, LUA_REGISTRYINDEX, handle_i );
    
    
    status_i=res->status;
    lua_pushinteger(L,status_i);
    lua_setfield(L, -2, "status");

    if(status_i){
      lua_rawseti(L, result_table, index);
      continue;
    }
    code_i=ntohl(res->code);
    lua_pushinteger(L, code_i);
    lua_setfield(L, -2, "code");
    uint32_t offset=ntohl(res->body_offset);
    uint32_t len=ntohl(res->data_len);
    
    char *ptr=res->data;
    char *end=ptr+offset;
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

    lua_pushlstring(L, res->data+offset,len-offset);
    lua_setfield(L, -2, "body");
    zmq_msg_close(&msg);

    lua_rawseti(L, result_table, index);
  }
}


void myfree(void *data, void *hint){

  free(data);
}

inline void raw_send_http_request(void *socket){
  uint32_t data_len=request.host->write_pos + request.path->write_pos + request.headers->write_pos + request.body->write_pos;
  uint32_t size=sizeof(packed_req_info)+data_len;
  zmq_msg_t msg;
  packed_req_info *req=malloc(size);
  req->data_len=htonl(data_len);
  req->method=request.method;
  req->port=request.port;
  req->handle=request.handle;
  req->options=request.options;
  char *p=req->data;
  uint32_t offset=0;
  memcpy(p, request.host->ptr, request.host->write_pos); offset+=request.host->write_pos;

  req->path_offset=htonl(offset);
  memcpy(p+offset, request.path->ptr, request.path->write_pos); offset+=request.path->write_pos;
  

  req->headers_offset=htonl(offset);
  memcpy(p+offset, request.headers->ptr, request.headers->write_pos); offset+=request.headers->write_pos;

  req->body_offset=htonl(offset);
  memcpy(p+offset, request.body->ptr, request.body->write_pos); offset+=request.headers->write_pos;

  zmq_msg_init_data(&msg,req,size,myfree,NULL);
  zmq_msg_send(&msg, socket, 0);
  
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
  request.handle=luaL_ref(L, LUA_REGISTRYINDEX );
  
  
  reset_simple_buffer(request.host);
  lua_getfield (L, 1, "host");
  write_string_to_simple_buffer(request.host, lua_tostring(L, -1));
  lua_pop(L,1);
  
  lua_getfield (L, 1, "method");
  request.method=htons(lua_tointeger (L, -1));
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
  request.port=htons(lua_tointeger (L, -1));
  lua_pop(L,1);

  lua_getfield (L, 1, "options");
  request.options=htons(lua_tointeger (L, -1));
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




