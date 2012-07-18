#include <stdio.h>
#include <gmp.h>
#include <string.h>
#include <sys/types.h>
#include "engine.h"
#include "bayes.h"
#include "util.h"
#include "match_rule.h"
#include "detect_404.h"
#include "post.h"
#include "utlist.h"

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

void output_result(struct http_request *req, struct http_response *res){
  struct annotation *a;
  char line[80];
  int size=0; 
  if(GET_HDR((unsigned char *) "content-length", &res->hdr)) size=atoi((char *) GET_HDR((unsigned char *) "content-length", &res->hdr));
  else size=res->pay_len;


  snprintf(line, 80, "[%d] %s",res->code,serialize_path(req,1,0));
  printf("%-75s %6d %-16s",line, size, res->header_mime);
  for(a=res->annotations;a;a=a->next){
    printf(" [%s",a->key);
    if(a->value) printf("=%s", a->value);
    printf("]");
  }
  printf("\n");
  fflush(stdout);

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

/* regular expression replace */

int rreplace (char *buf, int size, regex_t *re, char *rp)
{
  char *pos;
  int sub, so, n;
  regmatch_t pmatch [10]; /* regoff_t is int so size is int */
  if (regexec (re, buf, 10, pmatch, 0)) return 0;
  for (pos = rp; *pos; pos++)
    if (*pos == '\\' && *(pos + 1) > '0' && *(pos + 1) <= '9') {
      so = pmatch [*(pos + 1) - 48].rm_so;
      n = pmatch [*(pos + 1) - 48].rm_eo - so;
      if (so < 0 || strlen (rp) + n - 1 > size) return 1;
      memmove (pos + n, pos + 2, strlen (pos) - 1);
      memmove (pos, buf + so, n);
      pos = pos + n - 2;
    }
  sub = pmatch [1].rm_so; /* no repeated replace when sub >= 0 */
  for (pos = buf; !regexec (re, pos, 1, pmatch, 0); ) {
    n = pmatch [0].rm_eo - pmatch [0].rm_so;
    pos += pmatch [0].rm_so;
    if (strlen (buf) - n + strlen (rp) + 1 > size) return 1;
    memmove (pos + strlen (rp), pos + n, strlen (pos) - n + 1);
    memmove (pos, rp, strlen (rp));
    pos += strlen (rp);
    if (sub >= 0) break;
  }
  return 0;
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

void process_features(struct http_response *res, struct target *t){
  unsigned char *server=GET_HDR((unsigned char *) "server",&res->hdr);
  unsigned char *powered=GET_HDR((unsigned char *) "x-powered-by",&res->hdr);
  char *label;
  char *f_str;
  int i, size, server_l;
  if(!server){
    return;
  }
  server_l=strlen((char *) server);
  size=server_l+1;
  if(powered) size+=strlen((char *) powered)+1;
  f_str=ck_alloc(size);
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
  ck_free(f_str);
}


void dir_update_score(struct req_pointer *pointer, double f){
  struct dir_link_res *as_parent;
  struct http_request *r=pointer->req;
  struct req_pointer *ptr2;
  if(f==1.0) r->score=calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,1);
  else if(f==0.0) r->score=calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,0);
  else {
    double s1=f*calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,1);
    double s2=(1-f)*calculate_new_posterior(r->score,r->link->child_success,r->link->count,r->link->parent_child_success,r->link->parent_success,0);
    r->score=s1+s2;
    if(r->test->parent){
      HASH_FIND_INT(r->t->link_map,&r->test->id,as_parent);    
      if(r->test->children){      
        for(ptr2=as_parent->children;ptr2!=NULL;ptr2=ptr2->next){
          dir_update_score(ptr2, r->score);
        }
      }
    }
  }
}

u8 process_test_result(struct http_request *req, struct http_response *res){
  struct url_test *test=req->test;;
  struct feature_node *f;
  struct feature_test_result *ftr;
  struct dir_link_res *as_parent=NULL;
  struct req_pointer *pointer=NULL;
  int success=0;
  int rc=0;
  rc=is_404(req->t->detect_404,req,res);
  if(rc==DETECT_UNKNOWN){
    if(req->pointer) req->pointer->done=1;
    return 0;
  }
  if(!test) fatal("We should never be here without a test");
  success=(rc==DETECT_SUCCESS);

  /* parent dir */

  if(test->children){
    HASH_FIND_INT(req->t->link_map,&test->id,as_parent);
    as_parent->tested=1;
    if(success) as_parent->success=1;
    
    for(pointer=as_parent->children;pointer!=NULL;pointer=pointer->next){
      maybe_update_dir_link(pointer);
      if(!train && !pointer->done){
        dir_update_score(pointer,(double) success);
      }
    }
    if(!train) sort_host_queue(req->t->host);
  }

  /* child */

  if(test->parent){
    pointer=req->pointer;
    pointer->tested=1;
    pointer->success=success;
    if(pointer==NULL) fatal("req pointer is null!");
    maybe_update_dir_link(pointer);
    pointer->done=1;
  }

  if(req->no_url_save) return 0;

  test->count++;
  test->dirty=1;
  
  if(success){
      test->success++;
      run_post_rules(req,res);
      output_result(req, res);
  }
  DL_FOREACH(req->t->features,f) {
    ftr=find_or_create_ftr(test->id,f->data->id);
    ftr->count++;
    ftr->dirty=1;
    if(success) ftr->success++;
  }
  return 0;
}

struct target *add_target(u8 *host){
  u8 *url;
  struct target *t=(struct target *) ck_alloc(sizeof(struct target));
  struct http_request *first=(struct http_request *) ck_alloc(sizeof(struct http_request));
  if(!host) fatal("host is null");
  trim((char *) host);
  if(strlen((char *) host)==0) fatal("host is empty");
  
  url=calloc(strlen((char *) host)+10,1);
  if(strstr((char *) host,"http")!=(char *) host) strcat((char *) url,"http://");
  strcat((char *) url,(char *) host);
  if(host[strlen((char *) host)-1]!='/') strcat((char *) url,"/");
  t->features=NULL;
  t->test_scores=NULL;
  t->full_host=url;
  HASH_ADD_KEYPTR( hh, targets, t->full_host, strlen((char *) t->full_host), t );
  parse_url(url,first,NULL);
  
  if(first->host==NULL){
    info("couldn't parse host for '%s', skipping", url);
    return NULL;
  }
  t->host=ck_strdup(host);
  t->prototype_request=req_copy(first,0);
  t->prototype_request->method=ck_alloc(5);
  memcpy(t->prototype_request->method,"HEAD",5);
  t->detect_404=ck_alloc(sizeof(struct detect_404_info));
  add_default_rules(t->detect_404);
  first->callback=process_first_page;
  first->t=t;
  async_request(first);
  t->after_probes=enqueue_tests;
  return t;
}

struct http_request *new_request(struct target *t){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(t->prototype_request->method);
  return r;
}



struct http_request *new_request_with_method(struct target *t, unsigned char *method){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(method);
  return r;
}


struct http_request *new_request_with_method_and_path(struct target *t, unsigned char *method,unsigned char *path){
  struct http_request *r=req_copy(t->prototype_request,0);
  r->t=t;
  r->method=ck_strdup(method);
  tokenize_path(path,r,0);
  return r;
}

unsigned char *target_and_test_to_url(struct target *t, struct url_test *test){
    unsigned char *f=serialize_path(t->prototype_request,1,0);
    unsigned char *ret=ck_alloc(strlen((char *) f)+strlen((char *) test->url)+1);
    strcat((char *) ret, (char *) f);
    strcat((char *) ret, (char *) test->url);
    return ret;
}

unsigned char *macro_expansion(const unsigned char *url){
    static regex_t *full_domain_regex=NULL;
    static regex_t *longest_domain_part_regex=NULL;
    int longest_c=0;
    char *longest=NULL, *temp=NULL;
    char *host_copy;
    
    if(!strstr((char *) url,"%HOSTNAME%") && !strstr((char *) url,"%BIGDOMAIN%")) return ck_strdup(url); /* nothing to do */

    struct http_request *r=ck_alloc(sizeof(struct http_request));
    parse_url(url,r,0);
    if(!full_domain_regex){
      full_domain_regex=malloc(sizeof(regex_t));
      regcomp(full_domain_regex, "%HOSTNAME%", 0);
    }

    if(!longest_domain_part_regex){
      longest_domain_part_regex=malloc(sizeof(regex_t));
      regcomp(longest_domain_part_regex, "%BIGDOMAIN%", 0);
    }
    int size=strlen(url)+100;
    char *url_copy=ck_alloc(size); /* plenty of space */
    memcpy(url_copy,url,strlen(url));
    rreplace (url_copy, size, full_domain_regex, (char *) r->host);
    host_copy=ck_strdup(r->host);
    temp=strtok(host_copy,".");
    while(temp){
      if(strlen(temp)>longest_c){
        longest=temp;
        longest_c=strlen(temp);
      }
      temp=strtok(NULL,".");
    }
    if(longest_c>0) rreplace (url_copy, size, longest_domain_part_regex, longest);
    ck_free(host_copy);
    ck_free(r);
    return (unsigned char *) url_copy;
}


unsigned char process_first_page(struct http_request *req, struct http_response *res){
  if(res->state==STATE_DNSERR || res->code==0){

    /* hambone detected fail immediately */
    info("dns error or coudn't reach url for host %s", req->host);
    return 0;
  }
  if(req->addr) req->t->prototype_request->addr=req->addr;
  process_features(res,req->t);
  add_features_from_triggers(res,req->t);
  launch_404_probes(req->t);
  return 0;
}

int score_sort(struct test_score *lhs, struct test_score *rhs){
  if (lhs->score > rhs->score) return -1;
  else if (lhs->score < rhs->score) return 1;
  return 0;
}


void maybe_update_dir_link(struct req_pointer *pointer){
  if(!(pointer->tested && pointer->parent->tested)) return;
  pointer->link->dirty=1;
  pointer->link->count++;
  if(pointer->success) pointer->link->child_success++;
  if(pointer->parent->success) pointer->link->parent_success++;
  if(pointer->parent->success && pointer->success) pointer->link->parent_child_success++;
}

void enqueue_tests(struct target *t){
  struct url_test *test,*parent_test;
  struct test_score *score;
  struct http_request *request;
  struct dir_link_res *dir_res;
  unsigned char *before_expansion; 
  struct dir_link_res *parent_res;
  unsigned char *url_cpy;
  unsigned int queued=0;
  unsigned char *method;
  for(test=get_tests();test!=NULL;test=test->hh.next){
    
    if(train && max_train_count && test->count>max_train_count){
      continue;
    }
    if(test->children){
      dir_res=ck_alloc(sizeof(struct dir_link_res));
      dir_res->parent_id=test->id;
      HASH_ADD_INT(t->link_map, parent_id, dir_res );    
    }
    
    score=ck_alloc(sizeof(struct test_score));
    score->test=test;
  
    if(train) score->score=rand(); /* if training randomize order to prevent bias */
    else score->score=get_test_probability(test,t);
    LL_PREPEND(t->test_scores,score); 
  }

  /* for only partial training, add parent dirs as well */

  if(train && max_train_count) {
     LL_FOREACH(t->test_scores,score) {
      if(score->test->parent){
        parent_res=NULL;
        HASH_FIND_INT(t->link_map,&score->test->parent->parent_id,parent_res);
        if(!parent_res){
          HASH_FIND_INT(get_tests(),&score->test->parent->parent_id,parent_test);
          dir_res=ck_alloc(sizeof(struct dir_link_res));
          dir_res->parent_id=parent_test->id;
          HASH_ADD_INT(t->link_map, parent_id, dir_res );    
          
          score=ck_alloc(sizeof(struct test_score));
          score->test=parent_test;
          score->no_url_save=1;
          score->score=rand();
          LL_PREPEND(t->test_scores,score);
        }
      }
    }
  }

  LL_SORT(t->test_scores,score_sort);
  LL_FOREACH(t->test_scores,score) {

    method=(unsigned char*) recommend_method(t->detect_404, score->test);
    if(method==RECOMMEND_SKIP) continue;
    request=new_request(t);
    before_expansion=target_and_test_to_url(t,score->test);
    url_cpy=macro_expansion(before_expansion);
    ck_free(before_expansion);

    parse_url(url_cpy, request, 0);
    ck_free(url_cpy);
    request->test=score->test;
    request->callback=process_test_result;
    request->method=ck_strdup(method);  
    request->score=score->score;
    request->no_url_save=score->no_url_save;
    parent_res=NULL;
    if(score->test->parent){
      struct req_pointer *pointer=ck_alloc(sizeof(struct req_pointer));
      pointer->req=request;
      request->pointer=pointer;
      pointer->link=score->test->parent;
      request->link=score->test->parent;
      HASH_FIND_INT(t->link_map, &score->test->parent->parent_id, parent_res);
      LL_APPEND(parent_res->children,pointer);
      pointer->parent=parent_res;
    }
    async_request(request);
    queued++;
  }
}



