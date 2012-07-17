#ifndef FASTERPUSSYCAT_POST_H
#define FASTERPUSSYCAT_POST_H
#include "http_client.h"
/* post.c */
struct annotation *annotate(struct http_response *res, char *key, char *value);
unsigned char index_of(struct http_request *req, struct http_response *res, void *data);
void run_post_rules(struct http_request *req, struct http_response *res);
void add_post_rules(void);

#endif
