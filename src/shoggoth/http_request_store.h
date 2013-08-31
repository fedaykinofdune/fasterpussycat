/* http_request_store.c */
void              http_request_store_add(http_request_t *req);
http_request_t *  http_request_store_get(uint32_t handle);
void              http_request_store_del(http_request_t *req);
