/* find_uploaded_file.c */
void start_find_uploaded_file(struct target *t);
void find_uploaded_file(struct target *t, unsigned char *file);
u8 process_find_uploaded_file(struct http_request *req, struct http_response *res);
