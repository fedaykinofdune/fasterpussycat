#ifndef FASTERPUSSYCAT_BACKUP_BRUTEFORCE_H
#define FASTERPUSSYCAT_BACkUP_BRUTEFORCE_H

#include "http_client.h"
#include "engine.h"

/* backup_bruteforce.c */
extern unsigned int backup_bruteforce_stop;
extern unsigned int backup_bruteforce_days_back;

void backup_bruteforce(struct target *t, unsigned char *full_url);
u8 process_backup_bruteforce(struct http_request *req, struct http_response *res);

#endif
