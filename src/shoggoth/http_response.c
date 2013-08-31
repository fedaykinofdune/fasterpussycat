http_response_t *http_response_alloc(){
  http_response_t *res=malloc(sizeof(http_response_t));
  res->headers=NULL;  
  return res;
}

void http_response_destroy(http_response_t *res){
  http_header_destroy_list(res->headers);
  free(res);
}

void http_response_unpack_headers(http_response_t *res){
  simple_buffer_t *headers=res->packed_headers;
  char *key;
  char *value;
  char *ptr=res->packed_headers->ptr;
  char *end_ptr=ptr+res->packed_headers->write_pos;
  http_header_t *header, *last=NULL;;
  while(ptr<end_ptr){
    key=ptr;
    ptr+=strlen(key)+1;
    value=ptr;
    ptr+=strlen(value)+1;
    header=http_header_alloc();
    header->no_free=1;
    header->key=key;
    header->value=value;
    if(!last) res->headers=header;
    else last->next=header;
    last=header;
  }

}
