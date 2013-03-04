#include <string.h>
#include <strings.h>
#include "http_response.h"
#include "connection.h"
#include "global_options.h"

#define prefix(_long, _short) \
    strncmp((char*)(_long), (char*)(_short), strlen((char *)(_short)))

#define case_prefix(_long, _short) \
    strncasecmp((char*)(_long), (char*)(_short), strlen((char *)(_short)))


void reset_http_response(http_response *response){
  response->code=0;
  response->must_close=0;
  response->body_offset=0;
  response->body_len=0;
  response->chunk_length=0;
  response->expect_crlf=0;
  response->expected_body_len=-1;
  response->chunked=0;
  response->compressed=0;
  reset_simple_buffer(response->headers);
}




http_response *alloc_http_response(){
  http_response *r=malloc(sizeof(http_response));
  r->headers=alloc_simple_buffer(opt.header_buffer_size);
  reset_http_response(r);
  return r;
}


inline enum parse_response_code parse_chunked(connection *conn){
  int more=(conn->state==READING);
  char *line;
  int line_size;

  simple_buffer *buf=conn->read_buffer;
  unsigned int chunk_len;
  int r_count;
  char *ptr;
  http_response *response=conn->response;
  while(1){

    if(response->chunk_length<=0){
      if(response->expect_crlf){
        line=read_line_from_simple_buffer(buf, &line_size);
        if(!line){
          if (more) { return NEED_MORE; }
          break;
        }
        response->expect_crlf=0;
      }
      line=read_line_from_simple_buffer(buf, &line_size);
      if (!line || sscanf((char*)line, "%x", &chunk_len) != 1) {
        if (more) { return NEED_MORE; }
        break;
      }
      if(chunk_len==0) return FINISHED;

      response->chunk_length=chunk_len;
    }
    ptr=read_from_simple_buffer(buf,response->chunk_length, &r_count);
    if(r_count==0){
      if(more) { return NEED_MORE;}
      break;
    }
    response->chunk_length-=r_count;
    write_to_simple_buffer(conn->aux_buffer,ptr,r_count);
    if(response->chunk_length<=0) response->expect_crlf=1;
  }
  return CONTINUE;
}



inline enum parse_response_code parse_result_code(connection *conn){
  int more=(conn->state==READING);
  char *line;
  int line_size;
  unsigned int http_ver;
  http_response *response=conn->response;
  simple_buffer *buf=conn->read_buffer;
  
  if(buf->write_pos<7 || prefix(buf->ptr, "HTTP/1.")) return more ? NEED_MORE : INVALID;
  line=read_line_from_simple_buffer(buf, &line_size);
  if(!line) return more ? NEED_MORE : INVALID;
    
  *(line+line_size)=0;
  if(line_size < 13 || sscanf((char*) line, "HTTP/1.%u %hu ", &http_ver, &response->code) != 2 || response->code < 100 || response->code > 999) {
      return INVALID;
  }

  *(line+line_size)='\n';
  if(response->code == 206) response->code=200;
  printf("parsed code %d\n", response->code);
  return CONTINUE;
}


inline enum parse_response_code parse_headers(connection *conn){
  int more=(conn->state==READING);
  char *line;
  int line_size;
  char *h_key, *h_value;
  http_response *response=conn->response;
  simple_buffer *buf=conn->read_buffer;
  size_t h_key_size, h_value_size, key_wpos, value_wpos;;
  while(1){
    line=read_line_from_simple_buffer(buf, &line_size);
    if(!line) return more ? NEED_MORE : INVALID;
    if(line_size==0 || (line_size==1 && *line=='\r')){
      /* hit the response body */
      printf("hit response body\n");
      response->body_offset=buf->read_pos;
      if(conn->request->options & OPT_EXPECT_NO_BODY) return FINISHED;
      break;
    }

    /* parse the actual header */

    char *colon_ptr;
    char *end = (line[line_size-1]=='\r') ? line+line_size-1 : line+line_size;
    colon_ptr=memchr(line, ':', line_size);
    if(!colon_ptr || colon_ptr+1>=end || *(colon_ptr+1)!=' ') continue; // not a valid header, skip it
    h_key=line;
    h_key_size=colon_ptr-line;
    h_value=colon_ptr+2;
    h_value_size=end-(colon_ptr+2);
    key_wpos=response->headers->write_pos;
    write_packed_string_to_simple_buffer(response->headers, h_key, h_key_size);
    value_wpos=response->headers->write_pos;
    write_packed_string_to_simple_buffer(response->headers, h_value, h_value_size);
    h_key=response->headers->ptr+key_wpos;
    h_value=response->headers->ptr+value_wpos;
    printf("header key: '%s' value: '%s'\n", h_key, h_value);

    /* deal with particular headers */

    if (!case_prefix(h_key, "Content-Length") && sscanf(h_value, "%d", &response->expected_body_len) == 1 && response->expected_body_len < 0) return INVALID;
    else if (!case_prefix(h_key, "Transfer-Encoding") && strcasestr(h_value,"chunked")) response->chunked =1; 
    else if (!case_prefix(h_key, "Content-Encoding") && (strcasestr(h_value, "deflate") || strcasestr(h_value, "gzip"))) response->compressed=1;
    else if (!case_prefix(h_key, "Connection" ) && strcasestr(h_value,"close")) response->must_close=1;
  }

  return CONTINUE;
}



inline enum parse_response_code parse_connection_response2(connection *conn){
  simple_buffer *buf=conn->read_buffer;
  http_response *response=conn->response;
  int more=(conn->state==READING);
  int rc;

  /* parse http result code */

  if(response->code==0 && (rc=parse_result_code(conn))!=CONTINUE) return rc;
  
  /* parse headers */
  
  if(!response->body_offset && (rc=parse_headers(conn))!=CONTINUE) return rc;
  
  /* handle payload */

  if(response->chunked && (rc=parse_chunked(conn))!=CONTINUE) return rc;
  
  else if(response->expected_body_len>0){
    int size=buf->write_pos-response->body_offset;
    printf("size %d len %d \n", size, response->expected_body_len);
    if(size>=response->expected_body_len) return FINISHED;
  }
  
  if(more) return NEED_MORE;
    
  return FINISHED;
  

}

enum parse_response_code parse_connection_response(connection *conn){
  enum parse_response_code rc=parse_connection_response2(conn);
  if(rc!=FINISHED) return rc;
  http_response *response=conn->response;
  if(!response->compressed && !response->chunked) { /* the simplest case */
    response->body_ptr=conn->read_buffer->ptr+response->body_offset;
    response->body_len=conn->read_buffer->write_pos-response->body_offset;
    return FINISHED;
  }
  else if(!response->compressed && response->chunked) {
    response->body_ptr=conn->aux_buffer->ptr;
    response->body_len=conn->aux_buffer->write_pos;
    return FINISHED;
  }
  return FINISHED;
}

