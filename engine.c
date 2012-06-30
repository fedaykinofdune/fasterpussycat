#include <gmp.h>
#include <string.h>
#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "util.h"

static struct target *targets=NULL;
int check_dir=1;
int check_cgi_bin=1;
int check_404_size=0;
int check_200_size=1;
unsigned int max_requests=0;
int train=0;
int force_save=0;
unsigned int max_train_count=0;
struct target *target_by_host(u8 *host){
  struct target *t;
  HASH_FIND_STR(targets, (char *) host, t);
  return t;
}

void add_feature_label_to_target(const char *label, struct target *t){
  struct feature_node *fn;
  DL_FOREACH(t->features, fn){
    if(!strcasecmp(fn->data->label, label)){
      return;
    }
  }
  fn=(struct feature_node *) ck_alloc(sizeof(struct feature_node));
  fn->data=find_or_create_feature_by_label(label);
  fn->data->count++;
  fn->data->dirty=1;
  DL_APPEND(t->features,fn);
}




void add_feature_to_target(struct feature *f, struct target *t){
  struct feature_node *fn;
  DL_FOREACH(t->features, fn){
    if(!strcasecmp(fn->data->label, f->label)){
      return;
    }
  }
  fn=(struct feature_node *) ck_alloc(sizeof(struct feature_node));
  fn->data=f;
  fn->data->count++;
  fn->data->dirty=1;
  DL_APPEND(t->features,fn);
}

void process_features(struct http_response *rep, struct target *t){
  unsigned char *server=GET_HDR((unsigned char *) "server",&rep->hdr);
  unsigned char *powered=GET_HDR((unsigned char *) "x-powered-by",&rep->hdr);
  char *label;
  char *f_str;
  int i, size, server_l;
  if(!server){
    return;
  }
  server_l=strlen((char *) server);
  size=server_l+1;
  if(powered) size+=strlen((char *) powered)+1;
  f_str=malloc(size);
  strcpy(f_str,(char *) server);
  if(powered){
     f_str[server_l]=' ';
     strcpy(&f_str[server_l+1],(char *) powered);
  }
  for(i=0;f_str[i];i++){
    if(f_str[i]<'A' || f_str[i]>'z'){
      f_str[i]=' ';
    }
  }
  label=strtok((char *) f_str," ");
  while(label!=NULL){
    if(strlen((char *) label)>2){
      add_feature_label_to_target(label, t);
    }
    label=strtok(NULL," ");
  }
  free(f_str);
}

char is_success(struct url_test *test, int code){
  return (code==200 || ((test->flags & F_DIRECTORY) && (code==401 || code==403)));
} 

u8 process_test_result(struct http_request *req, struct http_response *rep){
  int code=rep->code;
  struct url_test *test;
  struct feature_node *f;
  struct target *t=req->t;
  struct feature_test_result *ftr;
  int size=0;
  
  HASH_FIND_INT(get_tests(), &req->user_val, test ); 
  if(GET_HDR((unsigned char *) "content-length", &rep->hdr)) size=atoi(GET_HDR((unsigned char *) "content-length", &rep->hdr));
  if(size>0 && rep->code==200 && !(test->flags && F_DIRECTORY)){
    if(t->twohundred_size==size) t->twohundred_size_count++;
    else if(t->twohundred_size!=-1){ 
      t->twohundred_size=size;
      t->twohundred_size_count=0;
    }
    
  }
  if(t->twohundred_size_count>MAX_REPEATED_SIZE_COUNT && t->twohundred_size!=-1){
    warn("Over %d repeated 200 responses of size %d from %s... looks bogus, removing host",MAX_REPEATED_SIZE_COUNT,size,t->host);
    t->twohundred_size=-1;
    remove_host_from_queue(t->host);
  }
  if(is_404(rep,req->t)){
    code=404;
  }
  test->count++;
  test->dirty=1;
  
  if(is_success(test,code)){
      test->success++;
      printf("CODE: %d http://%s%s\n",code,req->t->host,test->url);
  }
  DL_FOREACH(req->t->features,f) {
    ftr=find_or_create_ftr(test->id,f->data->id);
    ftr->count++;
    ftr->dirty=1;
    if(is_success(test,code)) ftr->success++;
  }
  t->last_code=code;
  return 0;
}

int is_404(struct http_response *rep, struct target *t){
  unsigned char *loc;
  if(rep->code==404 || rep->code==0){
    return 1;
  }
  if(check_404_size && t->fourohfour_content_length>0
  && GET_HDR((unsigned char *) "content-length", &rep->hdr) 
  && atoi(GET_HDR((unsigned char *) "content-length", &rep->hdr))==t->fourohfour_content_length) return 1;
  if(t->fourohfour_detect_mode==DETECT_404_LOCATION){
    loc=GET_HDR((unsigned char *) "location", &rep->hdr);
    if(loc && !strcasecmp((char *) t->fourohfour_location,(char *) loc)){
      return 1;
    }
  }
  return 0;
}

void add_target(u8 *host){
  u32 cur_pos;
  u8 *url;
  struct target *t=(struct target *) ck_alloc(sizeof(struct target));
  struct http_request *first=(struct http_request *) ck_alloc(sizeof(struct http_request));
  t->host=host;
  t->fourohfour_response_count=0;
  t->fourohfour_detect_mode=DETECT_404_NONE;
  t->features=NULL;
  t->test_scores=NULL;
  HASH_ADD_KEYPTR( hh, targets, t->host, strlen((char *) t->host), t );
  NEW_STR(url,cur_pos);
  ADD_STR_DATA(url,cur_pos,(unsigned char *) "http://");
  ADD_STR_DATA(url,cur_pos,host);
  ADD_STR_DATA(url,cur_pos,"/");
  parse_url(url,first,NULL);
  t->prototype_request=req_copy(first,0);
  t->prototype_request->method=ck_alloc(5);
  memcpy(t->prototype_request->method,"HEAD",5);
  first->callback=process_first_page;
  first->t=t;
  async_request(first);
}

void gen_random(unsigned char *s, const int len) {
  int i;
  static const char alphanum[] =
    "abcdefghijklmnopqrstuvwxyz";

  for (i = 0; i < len; ++i) {
    s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  s[len] = 0;
}

inline struct http_request *new_request(struct target *t){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(t->prototype_request->method);
  return r;
}

unsigned char process_first_page(struct http_request *req, struct http_response *rep){
  int i;
  if(rep->state==STATE_DNSERR || rep->code==0){

    /* hambone detected fail immediately */

    return 0;
  }
  process_features(rep,req->t);
  add_features_from_triggers(rep,req->t);
  for(i=0;i<MAX_404_QUERIES;i++){
    enqueue_random_request(req->t,(i==0),(i==1),0,(i==2));
  }
  return 0;
}

int process_404(struct http_response *rep, struct target *t){
  unsigned char *loc;
  unsigned char *len;
  if(t->fourohfour_response_count==1){
    if(rep->code==404){
      t->fourohfour_detect_mode=DETECT_404_CODE;
      len=GET_HDR((unsigned char *) "content-length",&rep->hdr);
      if(len){
        t->fourohfour_content_length=atoi(len);
      }
      printf("detect 404 %d\n",t->fourohfour_content_length);
      return 1;
    }
    else {
      loc=(unsigned char *) GET_HDR((unsigned char *) "location",&rep->hdr);
      if(loc){
        printf("detect loc\n");
        t->fourohfour_location=(unsigned char *) strdup((char *) loc);
        t->fourohfour_detect_mode=DETECT_404_LOCATION;
      }
      return 1;
    }
    return 0;
  }
  switch(t->fourohfour_detect_mode){
      case DETECT_404_NONE:
        return 0;
        break;
      case DETECT_404_CODE:
        if(rep->code!=404){
          return 0;
        }
        break;
      case DETECT_404_LOCATION:
        if(GET_HDR((unsigned char *) "location", &rep->hdr)==NULL){
          return 0;
        }
        if(strcasecmp((char *) t->fourohfour_location,(char *) GET_HDR((unsigned char *) "location", &rep->hdr))){
          return 0;
        }
        break;
  }
  return 1;
}

int score_sort(struct test_score *lhs, struct test_score *rhs){
  if (lhs->score > rhs->score) return -1;
  else if (lhs->score < rhs->score) return 1;
  return 0;
}

void enqueue_tests(struct target *t){
  struct url_test *test;
  struct test_score *score;
  struct http_request *request;
  struct feature_node *fn;
  struct feature_test_result *ftr;
  unsigned char *url_cpy;
  unsigned int queued=0;
  for(test=get_tests();test!=NULL;test=test->hh.next){
    
    if(train && max_train_count && test->count>max_train_count){
      continue;
    }
    
    if(t->skip_dir && (test->flags && F_DIRECTORY)){
      continue;
    }

    if(t->skip_cgi_bin && (test->flags && F_CGI)){
      test->count++;
      test->dirty=1;
      DL_FOREACH(t->features,fn){
        ftr=find_or_create_ftr(test->id,fn->data->id);
        ftr->count++;
        ftr->dirty=1;
      }  
      continue;
    }
    score=ck_alloc(sizeof(struct test_score));
    score->test=test;
  
    if(train) score->score=rand(); /* if training randomize order to prevent bias */
    else score->score=get_test_probability(test,t);
    LL_PREPEND(t->test_scores,score); 
  }
  LL_SORT(t->test_scores,score_sort);
  LL_FOREACH(t->test_scores,score) {
     request=new_request(t);
     url_cpy=ck_alloc(strlen(score->test->url)+1);
     memcpy(url_cpy,score->test->url,strlen(score->test->url)+1);
     tokenize_path(url_cpy, request, 0);
     request->user_val=score->test->id;
     request->callback=process_test_result;
     async_request(request);
     queued++;
     if(max_requests && queued>=max_requests) break;
  }
}



u8 process_dir_request(struct http_request *req, struct http_response *rep){
  struct target *t=req->t;
  t->checks--;
  if(!process_404(rep,t)){
    printf("FAILED DIR DETECT %s\n",t->host);
    t->skip_dir=1;
  }
  maybe_enqueue_tests(t);
  
  return 0;
}

u8 process_cgi_check_request(struct http_request *req, struct http_response *rep){
  if(!(rep->code==200 || rep->code==403)){
    printf("FAILED CGI DETECT %s\n",req->t->host);
    req->t->skip_cgi_bin=1;
  }
  req->t->checks--;
  maybe_enqueue_tests(req->t);
  return 0;
}


void enqueue_cgi_bin_check(struct target *t){
  struct http_request *req=new_request(t);
  unsigned char *url_cpy=ck_strdup((unsigned char *) "/cgi-bin/");
  tokenize_path(url_cpy, req, 0);
  req->callback=process_cgi_check_request;
  async_request(req);
}


inline void maybe_enqueue_tests(struct target *t){
  if(!t->checks){
    enqueue_tests(t);
  }
}


void enqueue_checks(struct target *t){
  if(check_dir){
    t->checks++;
    enqueue_random_request(t,0,0,1,0);
  }
  if(check_cgi_bin){
    t->checks++;
    enqueue_cgi_bin_check(t);
  }
  maybe_enqueue_tests(t);
}



u8 process_random_request(struct http_request *req, struct http_response *rep){
  struct target *t=req->t;
  t->fourohfour_response_count++;
  if(!process_404(rep,t)){
    printf("FAILED 404 DETECT %s\n",t->host);
    t->fourohfour_detect_mode=DETECT_404_NONE;
    return 0;
  }
  if(t->fourohfour_response_count>=MAX_404_QUERIES && t->fourohfour_detect_mode!=DETECT_404_NONE){
    enqueue_checks(t);
  }
  return 0;
}

void enqueue_random_request(struct target *t, int slash,int php, int dir, int pl){
  struct http_request *random_req=new_request(t);
  u8 *random=ck_alloc(18);
  if(dir){
    random_req->callback=process_dir_request;
  }
  else{
    random_req->callback=process_random_request;
  }
  random[0]='/';
  gen_random(&random[1],16);
  if(slash || dir){
    random[8]='/';
    if(dir) random[9]=0;
  }
  if(php){
    memcpy(&random[7],".php",5);
  }

  if(pl){
    memcpy(&random[7],".pl",5);
  }
  tokenize_path(random,random_req,0);
  async_request(random_req);
}
