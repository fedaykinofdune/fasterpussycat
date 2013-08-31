http_header_t *http_header_alloc(){
  http_header_t *header=alloc(sizeof(http_header_t));
  header->no_free=0;
  header->key=NULL;
  header->value=NULL
}


void http_header_destroy(http_header_t *header){
  if(!header->no_free){
    free(header->key);
    free(header->value);
  }
  free(header);
}

void http_header_destroy_list(http_header_t **list){
  current=*list;
  while(current){
    next=current->next;
    http_header_destroy(current);
    current=next;
  }
  *list=NULL;
}

void http_header_set_header(http_header_t **list, char *key, char *value){
  
  http_header_t *current=*list, *new, *next;;
  while(current){
    next=current->next;
    if(!strcmpcase(current->key, key)){
      if(current->value) free(current->value);
      if(value) current->value=strdup(value);
      else current->value=NULL;
        
      return;
    }
    if(!next) break;
    current=next;
  }
  
  new=http_header_alloc();
  new->key=strdup(key);
  new->value=strdup(value);
  if(*list) *list=new;
  else current->next=new;
}



const char *http_header_get_header(http_header_t **list, const char *key){
  http_header_t *current=*list;
  while(current){
    if(!strcmpcase(current->key, key)){
      return current->value;
    }
    current=current->next;
  }
  return NULL;  
}

