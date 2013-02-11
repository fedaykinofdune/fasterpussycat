#include <netdb.h>
#include <sys/socket.h> 
#include "address_map.h"

static resolved_address address_map=NULL;

void lookup_address(char *host, resolved_address_callback callback , prepared_http_request *req){
  resolved_address *r;
  HASH_FIND_STR(address_map, host, r);

  /* if the host is cached callback immediately */

  if(r!=NULL){


    callback(r->state,r->addr,req);
    goto do_callback;
  }

  /* otherwise do the name lookup */
  /* TODO: asynchronous name lookups */


  r=malloc(sizeof(resolved_address));
  r->state=ADDRESS_INPROGRESS;
  r->host=strdup(host);
  r->addr=0;
  HASH_ADD_KEYPTR( hh, address_map, r->host, strlen(r->host), r );
  struct hostent *h=gethostbyname(host);
  if(h==NULL){
    /* TODO: handle errors properly */
    r->state=ADDRESS_NOTEXIST;
    goto do_callback;
  }
  r->addr=(unsigned int *) h->h_addr;
  r->state=ADDRESS_FOUND
  free(h);

do_callback:

  callback(r->state,r->addr,data);
  }
