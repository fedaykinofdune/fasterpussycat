enum resolved_address_state { ADDRESS_RESOLVED, ADDRESS_NOTFOUND, ADDRESS_INPROGRESS }
typedef struct resolved_address resolved_address;
typedef void (*resolved_address_callback)(resolved_address_state state, unsigned int addr, prepared_http_request *req);
struct resolved_address {
  char *host;
  char addr[4];
  resolved_address_state state;
  prepared_http_request *unprocessed;
  UT_hash_handle hh;
};


