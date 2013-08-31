#include <errno.h>
#include <zmq.h>
#include <string.h>
#include <libgen.h>
#include <lua5.1/lua.h>
#include <lua5.1/lauxlib.h>
#include <lua5.1/lualib.h>
#include "common/simple_buffer.h"
#include "shoggoth_api.h"
#include "lua_helpers.h"

#define MAX_SOCKETS 32
#define POLL_TIMEOUT 100000

static simple_buffer *payload_to_send;

static zmq_pollitem_t pollitems[MAX_SOCKETS];
static void *sockets[MAX_SOCKETS];
static int n_pollitems=0;
static int next_socket;
static void *context;
static const struct luaL_reg api [] = {
        {"enqueue_http_request", l_enqueue_http_request},
        {"poll", l_poll},
        {"connect_endpoint", l_connect_endpoint},
        {"basename", l_basename},
        {"dirname", l_dirname},
        {"url_encode", l_url_encode},
        {"base64_encode", l_base64_encode},
        {"base64_decode", l_base64_decode},
        {NULL, NULL}  /* sentinel */

};

/* Converts an integer value to its hex character*/
char to_hex(char code) {
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *shog_url_encode(const char *str) {
  const char *pstr = str;
  char *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
      *pbuf++ = *pstr;
    else if (*pstr == ' ') 
      *pbuf++ = '+';
    else 
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';
  return buf;
}


void shog_initialize(){
  context=zmq_init (1);
  payload_to_send=simple_buffer_alloc(128);
}

void shog_enqueue_request(tcp_endpoint_t *end, http_request_t *req){
  
  void *socket=get_zmq_socket_to_use(end);
  zmq_msg_t header_msg;
  zmq_msg_t body_msg;
  unsigned char[27] header;
  int opts=htonl(req->opts | end->use_ssl);
  sockaddr_serialize(&(end->addr), header);
  memcpy(header+19,&opts,4);
  memcpy(header+23,&(req->handle),4);
  
  simple_buffer *payload;
  if(!req->payload){
    simple_buffer_reset(payload_to_send);
    http_request_write_to_simple_buffer(req, payload_to_send);
    payload=payload_to_send;
  }
  else payload=req->payload;
  zmq_msg_init_size(&header_msg,27);
  zmq_msg_init_size(&body_msg,payload->write_pos);
  memcpy(zmq_data(&header_msg), header, 27);
  memcpy(zmq_data(&body_msg), payload->data, payload->write_pos); 
  zmq_send(socket, &header_msg, ZMQ_SNDMORE);
  zmq_send(socket, &body_msg, 0);
  http_request_store_add(req);
}



int l_url_encode(lua_State *L){
  const char *src=lua_tostring(L,1);
  char *dest=url_encode(src);
  lua_pushstring(L,dest);
  free(dest);
  return 1;
}

int l_basename(lua_State *L){
  char *name=strdup(lua_tostring(L,1));
  lua_pushstring(L,basename(name));
  free(name);
  return 1;
}


int l_base64_encode(lua_State *L){
  int i;
  const char *from=lua_tostring(L,1);
  i=strlen(from);
  char *to=malloc(i*4);
  base64_encode(to,from,i);
  lua_pushstring(L,to);
  free(to);
  return 1;
}



int l_base64_decode(lua_State *L){
  int i;
  const char *from=lua_tostring(L,1);
  i=strlen(from);
  char *to=malloc(i);
  base64_decode(to,from,i);
  lua_pushstring(L,to);
  free(to);
  return 1;
}



int l_dirname(lua_State *L){
  char *name=strdup(lua_tostring(L,1));
  lua_pushstring(L,dirname(name));
  free(name);
  return 1;
}

void *shog_connect_endpoint(char *endpoint){
  void *sock;
  sock = zmq_socket (context, ZMQ_DEALER);
  if(zmq_connect (sock, endpoint)){
      warnx("Could not connect to endpoint %s", endpoint);
  }
  sockets[n_pollitems]=sock;
  pollitems[n_pollitems].socket=sock;
  pollitems[n_pollitems].fd=0;
  pollitems[n_pollitems].events=ZMQ_POLLIN;
  n_pollitems++;
}

int l_connect_endpoint(lua_State *L){
  const char *endpoint=lua_tostring(L,1);
  return 0;
}

http_response_t *shoggoth_poll(){
  static int last_item=0;
  zmq_poll(pollitems, n_pollitems, POLL_TIMEOUT);
  http_response_t *rep;
  int i;
  for(i=0;i<n_pollitems;i++){
    if(pollitems[i].revents & ZMQ_POLLIN){
      if(rep=raw_poll(pollitems[(i + last_item) % n_poll_items].socket)){
        last_item=i+1;
        return rep;
      }
    }
  }
  return NULL;
}



inline void *get_zmq_socket_to_use(http_endpoint_t *e){
  return sockets[(++next_socket % n_pollitems)];
}

http_response_t *raw_poll(void *sock){

  zmq_msg_t msg;
  zmq_msg_t headers;
  zmq_msg_t body;
  http_response_t *res;
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
  unsigned char *header[SHOG_SOCKSTORAGESIZE+8];
  while(1){
    index++;
    if (zmq_getsockopt (sock, ZMQ_EVENTS, &zmq_events, &zmq_events_size) == -1) {
      perror("something is fucked ;_;");
      abort();
    }
    if(!(zmq_events & ZMQ_POLLIN)){
      return;
    }
    int mixed;
    res=http_response_alloc();
    zmq_recv(sock,&res->handle,sizeof(uint32_t), 0);
    zmq_recv(sock,&mixed,sizeof(uint32_t), 0);
    mixed=ntohl(mixed);
    res->code=(mixed & 0x00FF);
    res->status=(mixed >> 16);
    zmq_msg_init(headers);
    size=zmq_msg_recv(sock, &headers);
    res->packed_headers=simple_buffer_dup_from_ptr(zmq_msg_data(&headers), size);
    zmq_msg_close(&headers);
    zmq_msg_init(&body);
    size=zmq_msg_recv(sock,&body);
    res->body=malloc(size+1);
    res->body_len=size;
    memcpy(res->body,zmq_msg_data(body), size);
    res->body[size]=0;
    zmq_msg_close(&body);
    http_response_unpack_headers(res);
    

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
    code_i=ntohs(res->code);
    lua_pushinteger(L, code_i);
    lua_setfield(L, -2, "code");
    uint32_t offset=ntohl(res->body_offset);
    uint32_t len=ntohl(res->data_len);
    
    char *ptr=res->data;
    char *end=ptr+offset;
    char *key;
    char *value;
    lua_newtable(L);
    char *p;
    while(ptr<end){
      key=ptr;
      ptr=ptr+strlen(key)+1;
      value=ptr;
      ptr=ptr+strlen(value)+1;
      
      lua_pushstring(L,value);
      for(p=key;p;p++) *p=tolower(*p);
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
  request.options=htonl(lua_tointeger (L, -1));
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




