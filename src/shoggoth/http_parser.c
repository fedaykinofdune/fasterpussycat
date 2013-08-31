#include <string.h>
#include <strings.h>
#include "http_response_parser.h"
#include "connection.h"
#include "global_options.h"
#include "common/zeromq_common.h"

#define prefix(_long, _short) \
    strncmp((char*)(_long), (char*)(_short), strlen((char *)(_short)))

#define case_prefix(_long, _short) \
    strncasecmp((char*)(_long), (char*)(_short), strlen((char *)(_short)))


void http_parser_reset(http_parser_t *parser){
  parser->code=0;
  parser->must_close=0;
  parser->body_offset=0;
  parser->body_len=0;
  parser->chunk_length=0;
  parser->expect_crlf=0;
  parser->expected_body_len=-1;
  parser->chunked=0;
  parser->compressed=0;
  parser->last_z_offset=0;
  simple_buffer_reset(parser->headers);
  simple_buffer_reset(parser->read_buffer);
  simple_buffer_reset(parser->aux_buffer);

}




http_parser_t *http_parser_alloc(){
  http_parser_t *r=malloc(sizeof(http_response_parser_t));
  r->headers=simple_buffer_alloc(opt.header_buffer_size);
  r->read_buffer=simple_buffer_alloc(opt.read_buffer_size);
  r->aux_buffer=simple_buffer_alloc(opt.aux_buffer_size);
  r->z_stream_initialized=0;
  http_response_parser_reset((t);
  return r;
}


void http_parser_ready_zstream(http_parser_t *parser){
   parser->z_stream.zalloc = Z_NULL;
   parser->z_stream.zfree = Z_NULL;
   parser->z_stream.opaque = Z_NULL;
   parser->z_stream.avail_in = 0;
   parser->z_stream.next_in = Z_NULL;
   inflateInit2(&parser->z_stream, 32+15);
   parser->z_stream_initialized=1;
}


void http_parser_destroy_zstream(http_parser_t *parser){
   if(parser->z_stream_initialized) inflateEnd(&parser->z_stream);
   parser->z_stream_initialized=0;
}

int http_parser_uncompress_to_aux_buffer(http_parser_t *parser, char *ptr, size_t size){
  simple_buffer *aux=parser->aux_buffer;
  parser->z_stream.avail_in=size;
  if(aux->size - aux->write_pos<16){
    aux->size*=2;
    aux->ptr=realloc(ptr, aux->size);
  }
  int ao=aux->size - aux->write_pos;

  parser->z_stream.avail_out=ao;
  parser->z_stream.next_in=ptr;
  parser->z_stream.next_out=aux->ptr;
  int ret = inflate(&(parser->z_stream), Z_NO_FLUSH);
  switch (ret) {
    case Z_NEED_DICT:
      printf("need dict\n");
      break;
    case Z_DATA_ERROR:
      printf("data error\n");
      break;
    case Z_MEM_ERROR:
      printf("mem error\n");;
      break;
  }
  int have = ao - parser->z_stream.avail_out;
  aux->write_pos+=have;
  return ret;
}

int http_parser_process_unchunked_compressed_connection(http_parser_t *parser){
  if(parser->last_z_offset==0) parser->last_z_offset=parser->body_offset;
  int rc=http_parser_uncompress_to_aux_buffer(parser, parser->read_buffer->ptr + parser->last_z_offset, parser->read_buffer -> write_pos - parser->last_z_offset);
  parser->last_z_offset=parser->read_buffer->write_pos;
  return rc;
}

inline enum http_parser_parse_chunked(http_parser_t  *parser, connection_t *connection){
  int more=(connection->state==READING);
  char *line;
  int line_size;

  simple_buffer_t *buf=parser->read_buffer;
  unsigned int chunk_len;
  int r_count;
  char *ptr;
  while(1){

    if(parser->chunk_length<=0){
      if(parser->expect_crlf){
        line=simple_buffer_read_line(buf, &line_size);
        if(!line){
          if (more) { return NEED_MORE; }
          break;
        }
        parser->expect_crlf=0;
      }
      line=simple_buffer_read_line(buf, &line_size);
      if (!line || sscanf((char*)line, "%x", &chunk_len) != 1) {
        if (more) { return NEED_MORE; }
        break;
      }
      if(chunk_len==0) return FINISHED;

      parser->chunk_length=chunk_len;
    }
    ptr=simple_buffer_read(buf,parser->chunk_length, &r_count);
    if(r_count==0){
      if(more) { return NEED_MORE;}
      break;
    }
    parser->chunk_length-=r_count;
    if(!parser->compressed) simple_buffer_write(parser->aux_buffer,ptr,r_count); 
    else{ http_parser_uncompress_to_aux_buffer(parser, ptr, r_count); }
    if(parser->chunk_length<=0) parser->expect_crlf=1;
  }
  return CONTINUE;
}



inline enum http_parser_state http_parser_parse_code(http_parser_t *parser, connection_t *connection){
  int more=(connection->state==READING);
  char *line;
  char *p2;
  int line_size;
  unsigned int http_ver;
  simple_buffer_t *buf=parser->read_buffer;
  
  if(buf->write_pos<13) return more ? NEED_MORE : INVALID;
  line=simple_buffer_read_line(buf, &line_size);
  if(!line) return more ? NEED_MORE : INVALID;
    
  *(line+line_size)=0;
  if(line_size < 13) return INVALID;
  parser->code=atoi(line+9);
  
  if(parser->code < 100 || parser->code > 999) return INVALID;
  *(line+line_size)='\n';
  if(parser->code == 206) response->code=200;
  return CONTINUE;
}


void http_parser_fill_response(http_parser_t *parser, http_response_t *res){
  res->packed_headers=parser->packed_headers;
  res->body=parser->body_ptr;
  res->body_len=parser->body_len;
  res->code=parser->code;
}

inline enum http_parser_state http_parser_parse_headers(http_parser_t *parser, connection_t *connection){
  char t[20];
  char l;
  int more=(connection->state==READING);
  char *line,*p;
  int line_size;
  char *h_key, *h_value;
  simple_buffer_t *buf=parser->read_buffer;
  size_t h_key_size, h_value_size, key_wpos, value_wpos;;
  while(1){
    line=simple_buffer_read_line(buf, &line_size);
    if(!line) return more ? NEED_MORE : INVALID;
    if(line_size==0 || (line_size==1 && *line=='\r')){
      /* hit the response body */
      parser->body_offset=buf->read_pos;
      if(connection->request->options & OPT_EXPECT_NO_BODY) return FINISHED;
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

    /* deal with particular headers */
    l=tolower(h_key[0]);
    if(l=='c' || l=='t'){
      memcpy(t, h_key, 20);
      t[19]=0;
      for (p=t; *p; ++p) *p = tolower(*p);
      if (!prefix(t, "content-length") && (parser->expected_body_len=atoi(h_value))<0) return INVALID;
      else if (!prefix(t, "transfer-encoding") && strcasestr(h_value,"chunked")) parser->chunked =1; 
      else if (!prefix(t, "content-encoding") && (strcasestr(h_value, "deflate") || strcasestr(h_value, "gzip"))) {parser->compressed=1; http_parser_ready_zstream(parser); }
      else if (!prefix(t, "connection" ) && strcasestr(h_value,"close")) parser->must_close=1;
      }
    }

  return CONTINUE;
}


inline enum http_parser_state parse_connection_response2(http_parser_t *parser, connection_t *connection){
  simple_buffer *buf=parser->read_buffer;
  int more=(connection->state==READING);
  int rc;

  /* parse http result code */

  if(response->code==0 && (rc=http_parser_parse_result_code(parser, connection))!=CONTINUE) return rc;
  
  /* parse headers */
  
  if(!response->body_offset && (rc=http_parser_parse_headers(parser, connnection))!=CONTINUE) return rc;
  
  /* handle payload */

  if(response->chunked && (rc=http_parser_parse_chunked(parser, connection))!=CONTINUE) return rc; 
  else {
    if(response->compressed){
      http_parser_process_unchunked_compressed_connection(parser,connnection);
    }
    if(response->expected_body_len>0){
      int size=buf->write_pos-response->body_offset;
      if(size>=response->expected_body_len) return FINISHED;
    }
    }
  
  if(more) return NEED_MORE;
    
  return FINISHED;
  

}

enum http_parser_state http_parser_parse(http_parser_t *parser, connection_t *connection){
  enum http_parser_state rc=http_parser_parse2(parser, connnection);
  if(rc!=FINISHED) return rc;
  if(!parser->compressed && !parser->chunked) { /* the simplest case */
    parser->body_ptr=parser->read_buffer->ptr+response->body_offset;
    parser->body_len=parser->read_buffer->write_pos-response->body_offset;
  }
  else if(parser->compressed || parser->chunked) {
    parser->body_ptr=conn->aux_buffer->ptr;
    parser->body_len=conn->aux_buffer->write_pos;
  }
  return FINISHED;
}

