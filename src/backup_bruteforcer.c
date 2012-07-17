#include <time.h>
#include <gmp.h>
#include <string.h>
#include <sys/types.h>
#include <regex.h>
#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "util.h"
#include "match_rule.h"
#include "detect_404.h"
#include "post.h"

unsigned int backup_bruteforce_days_back=365;
unsigned int backup_bruteforce_stop=1;

void backup_bruteforce(struct target *t, unsigned char *full_url){
  static int day_in_seconds=60*60*24;
  int days_back;
  unsigned char *fin_url=ck_alloc(1000);
  struct http_request *r;
  static char *patterns[] = {"%Y%m%d.sql","%Y%m%d.tgz", "%Y%m%d.zip", "%Y%m%d.tar.gz", 
                           "%Y-%m-%d.sql","%Y-%m-%d.tgz", "%Y-%m-%d.zip", "%Y-%m-%d.tar.gz", 
                           "%BIGDOMAIN%-%Y-%m-%d.sql","%BIGDOMAIN%-%Y-%m-%d.tgz", "%BIGDOMAIN-%Y-%m-%d.zip", "BIGDOMAIN-%Y-%m-%d.tar.gz",
                           "%BIGDOMAIN%-%Y%m%d.sql","%BIGDOMAIN%-%Y%m%d.tgz", "%BIGDOMAIN%-%Y%m%d.zip", "%BIGDOMAIN%-%Y%m%d.tar.gz",
                           "%BIGDOMAIN%%Y%m%d.sql","%BIGDOMAIN%%Y%m%d.tgz", "%BIGDOMAIN%%Y%m%d.zip", "%BIGDOMAIN%%Y%m%d.tar.gz"
                           ,0};

  time_t now=time(NULL);
  unsigned char *cur_pattern;
  int c;
  char *before_expansion=ck_alloc(strlen((char*) full_url)+100);
  char *url,*pattern;

  /* queue all the requests */

  for(days_back=1;days_back<=backup_bruteforce_days_back;days_back++){
    time_t new=now-(days_back*day_in_seconds);
    c=0;
    before_expansion[0]=0;
    while(patterns[c]){
      pattern=patterns[c];
      if(strlen(pattern)>99) continue; /* nah */
      strcat(before_expansion,full_url);
      strcat(before_expansion,patterns[c]);
      url=macro_expansion(before_expansion);
      strftime(fin_url, 999, url, localtime(&new));
      r=new_request(t);
      parse_url(fin_url,r,0);
      r->callback=process_backup_bruteforce;
      async_request(r);
      ck_free(url);
    }
  }
  ck_free(fin_url);
  

}


u8 process_backup_bruteforce(struct http_request *req, struct http_response *res){
  if(is_404(req->t->detect_404,req,res)) return 0;
  annotate(res,"found-backup",NULL);
  output_result(req,res);
  if(backup_bruteforce_stop){
    struct queue_entry *q=find_host_queue(req->t->full_host),*n;
    while(q){
      n=q->next;
      if(!q->c && q->req->callback==process_backup_brute_force && !strcmp((char *) q->req->t->full_host, (char *) req->t->full_host)) destroy_unlink_queue(q);
      q=n;
    }
  }

}

