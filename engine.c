#include <gmp.h>
#include <string.h>
#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"

static struct target *targets=NULL;


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
  int i, size, server_l, power_l;
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
  struct feature_test_result *ftr;
  HASH_FIND_INT(get_tests(), &req->user_val, test ); 
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
  return 0;
}

int is_404(struct http_response *rep, struct target *t){
  unsigned char *loc;
  if(rep->code==404 || rep->code==0){
    return 1;
  }
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


unsigned char process_first_page(struct http_request *req, struct http_response *rep){
  int i;
  if(rep->state==STATE_DNSERR){
    return 0;
  }
  process_features(rep,req->t);
  add_features_from_triggers(rep,req->t);
  printf("code now %d\n",rep->code);
  for(i=0;i<MAX_404_QUERIES;i++){
    enqueue_random_request(req,(i==0),(i==1),0);
  }
  return 0;
}

int process_404(struct http_response *rep, struct target *t){
  unsigned char *loc;
  if(t->fourohfour_response_count==1){
    if(rep->code==404){
      t->fourohfour_detect_mode=DETECT_404_CODE;

      printf("detect 404\n");
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
  unsigned char *url_cpy;
  for(test=get_tests();test!=NULL;test=test->hh.next){
    if(t->skip_dir && (test->flags && F_DIRECTORY)){
      continue;
    }
    score=ck_alloc(sizeof(struct test_score));
    score->test=test;
    score->score=get_test_probability(test,t);
    LL_PREPEND(t->test_scores,score); 
  }
  LL_SORT(t->test_scores,score_sort);
  LL_FOREACH(t->test_scores,score) {
     printf("enqueue http://%s%s score: %f\n",t->host,score->test->url,score->score);
     request=req_copy(t->prototype_request,0);
     request->method=(unsigned char *) ck_alloc(5);
     memcpy(request->method,"HEAD",5);
     url_cpy=ck_alloc(strlen(score->test->url)+1);
     memcpy(url_cpy,score->test->url,strlen(score->test->url)+1);
     tokenize_path(url_cpy, request, 0);
     request->t=t;
     request->user_val=score->test->id;
     request->callback=process_test_result;
     async_request(request);
  }
}



u8 process_dir_request(struct http_request *req, struct http_response *rep){
  struct target *t=req->t;
  if(!process_404(rep,t)){
    printf("FAILED DIR DETECT %s\n",t->host);
    t->skip_dir=1;
  }
  enqueue_tests(t);
  
  return 0;
}


u8 process_random_request(struct http_request *req, struct http_response *rep){
  struct target *t=req->t;
  t->fourohfour_response_count++;
  printf("code %d\n", rep->code);
  if(!process_404(rep,t)){
    printf("FAILED 404 DETECT %s\n",t->host);
    t->fourohfour_detect_mode=DETECT_404_NONE;
    return 0;
  }
  if(t->fourohfour_response_count>=MAX_404_QUERIES && t->fourohfour_detect_mode!=DETECT_404_NONE){
    enqueue_random_request(req,0,0,1);
  }
  return 0;
}

void enqueue_random_request(struct http_request *orig, int slash,int php, int dir){
  struct http_request *random_req=req_copy(orig,0);
  u8 *random=ck_alloc(18);
  if(dir){
    random_req->callback=process_dir_request;
  }
  else{
    random_req->callback=process_random_request;
  }
  printf("method 1 %s",random_req->method);
  random_req->method=(unsigned char *) ck_alloc(5);
  memcpy(random_req->method,"HEAD",5);
  
  printf("method 2 %s",random_req->method);
  random[0]='/';
  gen_random(&random[1],16);
  if(slash || dir){
    random[7]='/';
    if(dir) random[8]=0;
  }
  if(php){
    memcpy(&random[7],".php",5);
  }
  printf("random path = %s\n",random);
  tokenize_path(random,random_req,0);
  random_req->t=orig->t;
  async_request(random_req);
}
