#ifndef SHOGGOTH_ADDRESS_MAP_H
#define SHOGGOTH_ADDDESS_MAP_H
#include "common/uthash.h"
#include "prepared_http_request.h"

enum resolved_address_state { ADDRESS_RESOLVED, ADDRESS_NOTFOUND, ADDRESS_INPROGRESS };
typedef struct resolved_address resolved_address;
typedef void (*resolved_address_callback)(enum resolved_address_state state, unsigned int addr, prepared_http_request *req);
struct resolved_address {
  char *host;
  unsigned int addr;
  enum resolved_address_state state;
  prepared_http_request *unprocessed;
  UT_hash_handle hh;
};

/* address_map.c */
void lookup_address(char *host, resolved_address_callback callback, prepared_http_request *req);

#endif


