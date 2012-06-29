#include <stdio.h>
#include "utlist.h"
#include <gmp.h>
#include "http_client.h"
#include "db.h"
#include "engine.h"

static mpq_t prob_feat_given_true;
static mpq_t prob_feat_given_false; 
static mpq_t neg_posterior; 
static mpq_t evidence;
static mpq_t one;
static mpq_t zero;
static mpq_t posterior;

void setup_bayes(){
  mpq_init(evidence);
  mpq_init(neg_posterior);
  mpq_init(one);
  mpq_init(zero);
  mpq_init(posterior);
  mpq_init(feat_given_fail_ratio);
  mpq_init(feat_given_succ_ratio);
  mpq_set_ui(one,1,1);

}



void calculate_new_posterior(mpq_t *posterior, int sample_true, int sample_size, int feature_and_sample_true, int feature_detected_count, int feature_detected ){
  if(ftr->count==0) return;
  if(mpq_equal(*posterior,zero)) return;
  if(mpq_equal(*posterior,one)) return;
 
  mpq_sub(neg_posterior,one,*posterior);
  unsigned int sample_false=sample_size-sample_true;
  unsigned int sample_false_and_feature=feature_detected - feature_and_sample_true;

  if(url_test_success!=0){
    mpq_set_ui(prob_feat_given_true, feature_and_sample_true, sample_true);
    mpq_canonicalize(prob_feat_given_true);
    if(!evidence_seen){
      mpq_sub(prob_feat_given_true, one, prob_feat_given_true);
    }
    mpq_mul(prob_feat_given_true,*posterior, prob_feat_given_true);
  }
  else{
    mpq_set_ui(feat_given_succ_ratio,0,1);
  }
  if(url_test_fail!=0){
    mpq_set_ui(prob_feat_given_false,sample_false_and_feature, sample_false);
    mpq_canonicalize(prob_feat_given_false);
    if(!evidence_seen){
      mpq_sub(prob_feat_given_false, one, prob_feat_given_false);
    }
    mpq_mul(prob_feat_given_false,neg_posterior, prob_feat_given_false);
  }
  else{
    mpq_set_ui(prob_feat_given_false,0,1);
  }
  mpq_add(evidence,prob_feat_given_false,prob_feat_given_true);
  mpq_div(*posterior,prob_feat_given_true,evidence);
}

inline int feature_seen(struct feature *f, struct target *t){
  struct feature *f2;
  struct feature_node *fn;
  DL_FOREACH(t->features,fn){
    f2=fn->data;
    if(f2==f){
      return 1;
    }
  }
  return 0;
}

double get_test_probability(struct url_test *test, struct target *t){
  struct feature *f;
  struct feature_test_result *ftr;
  mpq_set_ui(posterior,test->success,test->count);
  mpq_canonicalize(posterior);
  for(f=get_features();f!=NULL;f=f->hh.next){
    if(f->count<200) continue;
    ftr=find_or_create_ftr(test->id,f->id);
    calculate_new_posterior(&posterior,test->success,test->count,ftr->success,ftr->count,feature_seen(f,t));
  }
  return mpq_get_d(posterior);
}





