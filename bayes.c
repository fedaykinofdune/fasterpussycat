#include <math.h>
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

double positive_predictive_value(struct url_test *test, struct feature *f){
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  return (double) ftr->success / ftr->count;
  // return ((double) ftr->success / test->success) / (((double) ftr->count / test->count) + ((ftr->count - ftr->success) / (test->count - test->success)));
}



double negative_predictive_value(struct url_test *test, struct feature *f){
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  return ((double) (test->count-test->success)-(ftr->count-ftr->success))/(test->count-ftr->count);
  // return (double) (1-((ftr->count - ftr->success) / (test->count - test->success))) / ( (1-((ftr->count - ftr->success) / (test->count - test->success))) + (1-((double) ftr->success / test->success)));
}




double information(double prob){
  double p1;
  double p2;
  if(prob==0.0 || prob==1.0){
    return 0;
  } 
  return (prob*(-log2(prob)))+((1-prob)*(-log2(1-prob)));
}


double merit(struct url_test *test, struct feature *f){
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  double m;
  if(test->count==0) return 0.0;
  double pF=(double) ftr->count / test->count;
  double PA=(double) test->success / test->count;
  double PB=1 - PA;
  double base_info=information(PA);
  double not_f_count=test->count-ftr->count;
  double false_count=(test->count-test->success)-(ftr->count-ftr->success);
  double entropy_evidence;
  if(ftr->count!=0) entropy_evidence=information(ftr->success/ftr->count);
  else entropy_evidence=0.0;
  double entropy_not_evidence=information(false_count/not_f_count);
  double after_entropy=(pF*entropy_evidence)+((1-pF)*entropy_not_evidence);;
  //return base_info-after;
  // printf("%f %f %f\n",pa,pb,log(pa/pb));
  return base_info-after_entropy;

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


double merit2(struct url_test *test, struct feature *f){
  struct feature_test_result *ftr=find_or_create_ftr(test->id,f->id);
  double m;
  if(test->count==0) return 0.0;
  double pF=(double) ftr->count / test->count;
  double PA=(double) test->success / test->count;
  double PB=1 - PA;
  mpq_set_ui(posterior,test->success, test->count);
  mpq_canonicalize(posterior);
  calculate_new_posterior(&posterior, test, ftr, 1);
  double base_info=information(PA);
  double not_f_count=test->count-ftr->count;
  double false_count=(test->count-test->success)-(ftr->count-ftr->success);
  double entropy_evidence=information(mpq_get_d(posterior));
  
  mpq_set_ui(posterior,test->success, test->count);
  mpq_canonicalize(posterior);
  calculate_new_posterior(&posterior, test, ftr, 0);
  double entropy_not_evidence=information(mpq_get_d(posterior));
  double after_entropy=(pF*entropy_evidence)+((1-pF)*entropy_not_evidence);;
  //return base_info-after;
  // printf("%f %f %f\n",pa,pb,log(pa/pb));
  return base_info-after_entropy;

}

void show_feature_predictive_values(){
  struct feature *f;
  struct url_test *test;
  double ppv, npv, m, m2;
  for(f=get_features();f!=NULL;f=f->hh.next){
    if(f->count<50) continue;
    ppv=0;
    npv=0;
    m=0;
    m2=0;
    int count=0;
    for(test=get_tests();test!=NULL;test=test->hh.next){
      if(test->success<10) continue;
      m2=merit(test,f);
    //  if(isnan(m2)){
    //    m2=0;
    //  }
      m+=m2;
      ppv+=positive_predictive_value(test,f);
      npv+=negative_predictive_value(test,f);
      count++;
    }
    printf("%s ppv %f npv %f merit %f\n", f->label, ppv/count, npv/count, m);
  }
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





