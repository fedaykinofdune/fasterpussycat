


#define prefix(_long, _short, _size) \
    strncmp((char*)(_long), (char*)(_short), (int) _size)

#define case_prefix(_long, _short, _size) \
    strncasecmp((char*)(_long), (char*)(_short), (int) _size)


void reset_http_response(http_response *response){
  response->code=0;
  response->must_close=0;
  response->body_offset=0;
  response->body_len=0;
  response->expected_body_len=0;
  response->chunked=0;
  response->compressed=0;
}


parse_response_code parse_connection_response(connection *conn){
  simple_buffer *buf=conn->read_buffer;
  http_response response=conn->response;
  int more=(conn->state==READING);
  char *line;
  int line_size;
  int http_ver;
  if(response->code==0){
    if(buf->read_pos<7 || prefix(buf->ptr, "HTTP/1.", 7)) return more ? NEED_MORE : INVALID;
    line=read_line_from_simple_buffer(buf, &line_size);
    if(!line) return more ? NEED_MORE : INVALID;
    if(line_size < 13 || sscanf((char*) line, "HTTP/1.%u %u ", &http_ver, &response->code) != 2 || response->code < 100 || response->code > 999) {
      return INVALID;
     }
    if(response->code == 206) response->code=200;
  }
  if(!response->body_offset){
    
    /* read the headers */
    
    while(1){
      line=read_line_from_simple_buffer(buf, &line_size);
      if(!line) return more ? NEED_MORE : INVALID;
      if(line_size==0){
        /* hit the response body */
        if(conn->request->options & OPT_EXPECT_NO_BODY) return FINISHED;
        response->body_offset=buf->read_pos;
        break;
      }
      if (!case_prefix(line, "Content-Length:", line_size)) {
        *(line+line_size)=0;
        int rc=sscanf((char*) line + 15, "%d", &response->expected_body_length) == 1;
        *(line+line_size)="\n";
        if(rc);
          if (response->expected_body_length < 0) return INVALID;
        }
        else response->expected_body_length=-1;
      }
    else if (!case_prefix(cur_line, "Transfer-Encoding:")) {

      /* Transfer-Encoding: chunked must be accounted for to properly
         determine if we received all the data when Content-Length not found. */

      char *x = line + 18;
      while (x<line+line_size && isspace(*x)) x++;
      if (!strcasencmp((char*)x, "chunked", line_size-(x-line))) response->chunked = 1;

     }

     else if (!case_prefix(cur_line, "Content-Encoding:")) {

      /* Transfer-Encoding: chunked must be accounted for to properly
         determine if we received all the data when Content-Length not found. */

      char *x = line + 17;
      while (x<line+line_size && isspace(*x)) x++;
      if (!strncasecmp((char*)x, "deflate", line_size-(x-line)) || !strncasecmp((char*)x, "gzip", line_size-(x-line))  )  response->compressed = 1;

     }
     else if (!case_prefix(cur_line, "Connection:")) {

      char *x = line + 11;
      while (x<line+line_size && isspace(*x)) x++;
      if (!strncasecmp((char*)x, "close", line_size-(x-line)) )  response->must_close = 1;

     }

  }

  /* handle payload */

  if(response->chunked){
    unsigned int chunk_len;
    int r_count;
    char *ptr;
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
        read_line_from_simple_buffer(buf, &line_size);
        if (!line || sscanf((char*)line, "%x", &chunk_len) != 1) {
          if (more) { return NEED_MORE; }
          break;
        }
        if(chunk_len==0) return FINISHED;
    
        response->chunk_length=chunk_length;
      }
      ptr=read_from_simple_buffer(buf,response->chunk_length, &r_count);
      if(r_count==0){
        if(more) { return NEED_MORE;}
        break;
      }
      response->chunk_length-=r_count;
      write_to_simple_buffer(conn->aux_buffer,ptr,r_count);
      if(response->chunk_length<=0)
        response->expect_crlf=1;
      }
  }
  else if(response->expected_body_length>0){
    int size=buf->write_pos-response->body_offset;
    if(size>=response->expected_body_length){
      return FINISHED:
    }
    if(more){
      return NEED_MORE;
    }
    return FINISHED;
  }
  else{
    if(more) return NEED_MORE;
    return FINISHED:
  }

}
