#include "uthash.h"
#include "utlist.h"
#include "http_client.h"
#include "db.h"
#include "engine.h"
#include "bayes.h"
#include "id3.h"


int in_chain(struct feature *f, struct id3_node *root){
  struct id3_node *i;
  for(i=root;i!=NULL;i=i->parent){
    if(i->feature==f) return 1;
  }
  return 0;
}

int depth(struct id3_node *root){
  int depth=0;
  struct id3_node *i;
  for(i=root;i!=NULL;i=i->parent){
    depth++;
  }
  return depth;
}

void do_feature_selection(){
  struct url_test *t;
  struct id3_node *tree;
  struct feature *f;

  struct feature *top=NULL;
  struct feature_selection *fs=NULL;
  struct feature_selection *fs_node;
  int i=0;
  double current_highest=0;
  double highest=0;
  double gain=0;
  struct test_score score;
  for(t=get_tests();t!=NULL;t=t->hh.next){
      fs=NULL;
      score.score=(double) t->success/(double) t->count;
      score.test=t;
      current_highest=9000;

      /* if there are less than 100 successes for this test,
       * we probably don't know enough to make an informed decision.
       * Roll with the total probability */

      if(t->success<100){
        t->feature_selections=NULL;
        goto skip_feature_selection;
      }
      for(i=0;i<9;i++){
        highest=0;
        for(f=get_features();f!=NULL;f=f->hh.next){
          gain=expected_change(&score,f);
          if(gain>highest && gain<current_highest){
            highest=gain;
            top=f;
          }
        }
        current_highest=highest;
        fs_node=malloc(sizeof(struct feature_selection));
        fs_node->feature=top;
        fs_node->next=fs;
        fs=fs_node;
      }

skip_feature_selection:

      printf("features for %s :",t->url);
      for(fs_node=fs;fs_node!=NULL;fs_node=fs_node->next){
        if(fs_node->feature==NULL) continue;
        printf(" %s", fs_node->feature->label);
      }
      t->feature_selections=fs;
      update_feature_selection(t);
      printf("\n");
  }
}

void print_path(struct id3_node *node){
  if(node->parent) print_path(node->parent);
  switch(node->type){
    case ID3_ROOT:
      printf("[R]");
      break;
    case ID3_EVD:
      printf("[+]");
      break;
    case ID3_NO_EVD:
      printf("[-]");
      break;
  }
  if(node->parent){
    printf("%s ",node->parent->feature->label);
  }
}


struct feature_selection *collapse_tree(struct id3_node *current,struct feature_selection **head){
  struct feature_selection *n;
  if(!current->feature) return NULL;
  if(!head) {
    head=malloc(sizeof(struct feature_selection *));
    *head=malloc(sizeof(struct feature_selection));
    (*head)->feature=current->feature;
    (*head)->next=NULL;
  }
  if(current->child_no_evidence){

    for(n=(*head);n!=NULL;n=n->next){
      if (n->feature==current->child_no_evidence->feature) goto found1;
    }

    n=malloc(sizeof(struct feature_selection));
    n->next=*head;
    n->feature=current->child_no_evidence->feature;
    *head=n;

found1:

    collapse_tree(current->child_no_evidence,head);
  }

  if(current->child_evidence){
    for(n=*head;n!=NULL;n=n->next){
      if (n->feature==current->child_evidence->feature) goto found2;
    }
    n=malloc(sizeof(struct feature_selection));
    
    n->feature=current->child_evidence->feature;
    n->next=*head;
    *head=n;

found2:

     collapse_tree(current->child_no_evidence,head);
  }


  n=*head;
  if(current->parent==NULL) free(head);
  return n;
  
}

struct feature *pick_feature(struct id3_node *root){
  struct feature *f;
  struct feature *top=NULL;
  double highest=0;
  if(depth(root)>9){
    return NULL;
  }
  for(f=get_features();f!=NULL;f=f->hh.next){
    double gain=0;
    if(f->count<50) continue;
    if(in_chain(f,root)) continue;
    gain=expected_change(&root->score,f);
    if(gain>highest){
      top=f;
      highest=gain;
    }
  }
  
  return top;
}

struct id3_node *spawn_child(struct id3_node *parent, int evidence){
  struct id3_node *child=calloc(sizeof(struct id3_node),1);
  struct feature_test_result *ftr;
  struct url_test *test=parent->test;
  child->parent=parent;
  child->type=evidence;
  child->test=test;
  child->score.test=test;
  ftr=find_or_create_ftr(test->id,parent->feature->id);
  child->score.score=calculate_new_posterior(parent->score.score,test->success, test->count, ftr->success, ftr->count, evidence);
  return child;
}


struct id3_node *build_tree(struct url_test *test, struct id3_node *root){
  struct feature *picked;
  /* if the root node is null, construct a new root node */

  if(root==NULL){
    root=calloc(sizeof(struct id3_node),1);
    root->type=ID3_ROOT;
    root->test=test;
    root->score.test=test;
    root->score.score=(double) test->success / (double) test->count;
    return build_tree(test,root);
  }
  

  /* otherwise continue */

  picked=pick_feature(root);
  if(picked==NULL){
  //print_path(root->parent);
  //printf("[STOP]\n");
  return root; /* end of tree */
  }
  root->feature=picked;
  root->child_evidence=spawn_child(root,1);
  root->child_no_evidence=spawn_child(root,0);
  build_tree(test, root->child_evidence);
  build_tree(test, root->child_no_evidence);
  return root;




  
}




