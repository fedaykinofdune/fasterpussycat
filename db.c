#include <stdlib.h>   
#include <stddef.h> 
#include <sqlite3.h>
#include "debug.h"
#include "uthash.h"
#include "types.h"
#include "alloc-inl.h"
#include "string-inl.h"
#include "db.h"
#include "http_client.h"

#define GET_TESTS_SQL "SELECT id, url, description, code_200, code_301, code_302, code_401, code_403, code_500, code_other, count, flags FROM url_tests"

#define GET_TEST_BY_URL_SQL "SELECT id, url, description, code_200, code_301, code_302, code_401, code_403, code_500, code_other, count, flags FROM url_tests WHERE url=? LIMIT 1"
#define GET_FEATURES_SQL "SELECT id, label, count FROM features"
#define GET_FTR_BY_TEST_AND_FEATURE_SQL "SELECT id, url_test_id, feature_id, code_200, code_301, code_302, code_401, code_403, code_500, code_other, count FROM feature_test_results WHERE url_test_id=? AND feature_test_id=?"

#define GET_FTR_BY_FEATURE_SQL "SELECT id, url_test_id, feature_id, code_200, code_301, code_302, code_401, code_403, code_500, code_other, count FROM feature_test_results WHERE feature_test_id=?"  

#define INSERT_FEATURE_SQL "INSERT INTO features (label) VALUES (?)" 

#define UPDATE_FEATURE_SQL "UPDATE features SET count=? WHERE id=?" 

#define INSERT_FEATURE_TEST_RESULT_SQL "INSERT INTO feature_test_results (url_test_id,feature_id,code_200,code_301,code_302,code_401,code_403,code_500,code_other,count) VALUES (?,?,?,?,?,?,?,?,?,?)" 


#define UPDATE_FEATURE_TEST_RESULT_SQL "UPDATE feature_test_results SET code_200=?,code_301=?,code_302=?,code_401=?,code_403=?,code_500=?,code_other=?,count=? WHERE id =?" 

#define INSERT_URL_TEST_SQL "INSERT INTO url_tests (url,description,flags) VALUES (?,?,?)" 


#define UPDATE_URL_TEST_SQL "UPDATE url_tests SET description=?, code_200=?, code_301=?, code_302=?, code_401=?, code_403=?, code_500=?, code_other=?, count=?, flags=? WHERE id=?" 

static struct url_test *test_map=NULL;
static struct url_test *test_map_by_url=NULL;
static struct feature_test_result *result_map=NULL;
static struct feature *feature_map=NULL;
static sqlite3 *db;

static sqlite3_stmt *get_ftr_by_feature_stmt;
static sqlite3_stmt *get_tests_stmt;
static sqlite3_stmt *get_test_by_url_stmt;
static sqlite3_stmt *get_features_stmt;

static sqlite3_stmt *insert_feature_stmt;
static sqlite3_stmt *update_feature_stmt;
static sqlite3_stmt *insert_ftr_stmt;
static sqlite3_stmt *update_ftr_stmt;
static sqlite3_stmt *insert_url_test_stmt;
static sqlite3_stmt *update_url_test_stmt;



int open_database(){
  int rc=sqlite3_open("fasterpussycat.sqlite3", &db);
  if(rc){

    sqlite3_close(db);
    FATAL("bad db :(");
  }
  sqlite3_prepare_v2(db, GET_TESTS_SQL, -1, &get_tests_stmt,NULL);
  sqlite3_prepare_v2(db, GET_TEST_BY_URL_SQL, -1, &get_test_by_url_stmt,NULL);
  sqlite3_prepare_v2(db, GET_FTR_BY_FEATURE_SQL, -1, &get_ftr_by_feature_stmt,NULL);
  sqlite3_prepare_v2(db, GET_FEATURES_SQL, -1, &get_features_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_FEATURE_SQL, -1, &insert_feature_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_FEATURE_SQL, -1, &update_feature_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_FEATURE_TEST_RESULT_SQL, -1, &insert_ftr_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_FEATURE_TEST_RESULT_SQL, -1, &update_ftr_stmt,NULL);
  sqlite3_prepare_v2(db, INSERT_URL_TEST_SQL, -1, &insert_url_test_stmt,NULL);
  sqlite3_prepare_v2(db, UPDATE_URL_TEST_SQL, -1, &update_url_test_stmt,NULL);
  return 0;
}

struct url_test *get_tests(){
  return test_map;
}

struct feature *find_or_create_feature_by_label(const char *label){
  struct feature *result;
  int i;
  char *l_label=strdup(label);
  for (i = 0; l_label[i]; i++){
    l_label[i]=tolower(l_label[i]);
  }
  HASH_FIND_STR(feature_map, l_label, result);
  if(result!=NULL){
    free(l_label);
    return result;
  }

  result=ck_alloc(sizeof(struct feature));
  result->id=-1;
  result->label=l_label;
  result->dirty=1;
  save_feature(result);
  HASH_ADD_KEYPTR( hh, feature_map, result->label, strlen(result->label), result);
  return result; 
}

void save_feature(struct feature *f){
  if(f->id==-1){
    sqlite3_reset(insert_feature_stmt);
    sqlite3_bind_text(insert_feature_stmt,1,f->label, -1, SQLITE_TRANSIENT);
    if(sqlite3_step(insert_feature_stmt)!=SQLITE_DONE){
      printf("SOME KIND OF FAIL IN FEATURE INSERT");
      exit(-1);
    }
    f->id=sqlite3_last_insert_rowid(db);
    f->dirty=0;
  }
  else{
    sqlite3_reset(update_feature_stmt);
    sqlite3_bind_int(update_feature_stmt,1,f->count);
    sqlite3_bind_int(update_feature_stmt,2,f->id);
    if(sqlite3_step(update_feature_stmt)!=SQLITE_DONE){
      printf("SOME KIND OF FAIL IN FEATURE UPDATE");
      exit(-1);
    }
    f->dirty=0;
  }
  
}

void save_all(){
  struct feature *feature_iter; 
  struct url_test *url_test_iter;
  struct feature_test_result *ftr_iter;
  sqlite3_exec(db, "BEGIN", 0, 0, 0);

  for(feature_iter=feature_map; feature_iter!=NULL; feature_iter=feature_iter->hh.next){
    if(feature_iter->dirty){
      save_feature(feature_iter);
    }
  }

  for(url_test_iter=test_map; url_test_iter!=NULL; url_test_iter=url_test_iter->hh.next){
    if(url_test_iter->dirty){
      save_url_test(url_test_iter);
    }
  }

  for(ftr_iter=result_map; ftr_iter!=NULL; ftr_iter=ftr_iter->hh.next){
    if(ftr_iter->dirty){
      save_ftr(ftr_iter);
    }
  }
 sqlite3_exec(db, "COMMIT", 0, 0, 0);
}



void save_ftr(struct feature_test_result *f){
  if(f->id==-1){
    sqlite3_reset(insert_ftr_stmt);
    sqlite3_bind_int(insert_ftr_stmt,1,f->url_test_id);
    sqlite3_bind_int(insert_ftr_stmt,2,f->feature_id);
    sqlite3_bind_int(insert_ftr_stmt,3,f->code_200);
    sqlite3_bind_int(insert_ftr_stmt,4,f->code_301);
    sqlite3_bind_int(insert_ftr_stmt,5,f->code_302);
    sqlite3_bind_int(insert_ftr_stmt,6,f->code_401);
    sqlite3_bind_int(insert_ftr_stmt,7,f->code_403);
    sqlite3_bind_int(insert_ftr_stmt,8,f->code_500);
    sqlite3_bind_int(insert_ftr_stmt,9,f->code_other);
    sqlite3_bind_int(insert_ftr_stmt,10,f->count);
    if(sqlite3_step(insert_ftr_stmt)!=SQLITE_DONE){
      printf("SOME KIND OF FAIL IN FTR INSERT");
      exit(-1);
    }
    f->id=sqlite3_last_insert_rowid(db);
    f->dirty=0;
  }
  else{
    sqlite3_reset(update_ftr_stmt);

    sqlite3_bind_int(update_ftr_stmt,1,f->code_200);
    sqlite3_bind_int(update_ftr_stmt,2,f->code_301);
    sqlite3_bind_int(update_ftr_stmt,3,f->code_302);
    sqlite3_bind_int(update_ftr_stmt,4,f->code_401);
    sqlite3_bind_int(update_ftr_stmt,5,f->code_403);
    sqlite3_bind_int(update_ftr_stmt,6,f->code_500);
    sqlite3_bind_int(update_ftr_stmt,7,f->code_other);
    sqlite3_bind_int(update_ftr_stmt,8,f->count);
    sqlite3_bind_int(update_ftr_stmt,9,f->id);
    if(sqlite3_step(update_ftr_stmt)!=SQLITE_DONE){
      printf("SOME KIND OF FAIL IN FTR UPDATE");
      exit(-1);
    }
    f->dirty=0;
  }
  
}



void save_url_test(struct url_test *f){
  int rc;
  printf("save url test");
  if(f->id==-1){
    sqlite3_reset(insert_url_test_stmt);
    sqlite3_bind_text(insert_url_test_stmt,1,f->url,-1,SQLITE_STATIC);
    sqlite3_bind_text(insert_url_test_stmt,2,f->description,-1,SQLITE_STATIC);
    sqlite3_bind_int(insert_url_test_stmt,3,f->flags);
    if(sqlite3_step(insert_url_test_stmt)!=SQLITE_DONE){
      printf("SOME KIND OF FAIL IN URL TEST INSERT");
      exit(-1);
    }
    f->id=sqlite3_last_insert_rowid(db);
    f->dirty=0;
  }
  else{
    sqlite3_reset(update_url_test_stmt);

    sqlite3_bind_text(update_url_test_stmt,1,f->description,-1,SQLITE_STATIC);
    sqlite3_bind_int(update_url_test_stmt,2,f->code_200);
    sqlite3_bind_int(update_url_test_stmt,3,f->code_301);
    sqlite3_bind_int(update_url_test_stmt,4,f->code_302);
    sqlite3_bind_int(update_url_test_stmt,5,f->code_401);
    sqlite3_bind_int(update_url_test_stmt,6,f->code_403);
    sqlite3_bind_int(update_url_test_stmt,7,f->code_500);
    sqlite3_bind_int(update_url_test_stmt,8,f->code_other);
    sqlite3_bind_int(update_url_test_stmt,9,f->count);
    sqlite3_bind_int(update_url_test_stmt,10, f->flags);
    sqlite3_bind_int(update_url_test_stmt,11,f->id);
    
    if((rc=sqlite3_step(update_url_test_stmt))!=SQLITE_DONE){
      printf("%s\n",sqlite3_errmsg(db));
      printf("SOME KIND OF FAIL IN URL TEST UPDATE %d",rc);
      exit(-1);
    }
    f->dirty=0;
  }
  
}


struct feature_test_result *find_or_create_ftr(int url_test_id, int feature_id){
  struct feature_test_result *result;
  int keylen = offsetof(struct feature_test_result, feature_id) + sizeof(int) - offsetof(struct feature_test_result, url_test_id);
  struct ftr_lookup_key *lookup_key=ck_alloc(sizeof(struct ftr_lookup_key));
  lookup_key->url_test_id=url_test_id;
  lookup_key->feature_id=feature_id;
  HASH_FIND(hh, result_map, &lookup_key->url_test_id, keylen, result);
  ck_free(lookup_key);
  if(result!=NULL){
    return result;
  }
  result=ck_alloc(sizeof(struct feature_test_result));
  result->url_test_id=url_test_id;
  result->feature_id=feature_id;
  result->id=-1;
  result->dirty=1;
  HASH_ADD( hh, result_map, url_test_id, keylen, result );
  return result; 
}

int load_ftr_by_feature_id(int feature_id){
  sqlite3_reset(get_ftr_by_feature_stmt);
  sqlite3_bind_int(get_ftr_by_feature_stmt,1,feature_id);
  while (sqlite3_step(get_ftr_by_feature_stmt) == SQLITE_ROW){
    load_ftr();
  }
  return 0;
}



int load_features(){
  sqlite3_reset(get_features_stmt);
  while (sqlite3_step(get_features_stmt) == SQLITE_ROW){
    load_feature();
  }
  return 0;
}


int load_feature(){
  struct feature *feat;
  feat=ck_alloc(sizeof(struct feature));
  feat->id=sqlite3_column_int(get_features_stmt,0);
  feat->label=(char *) strdup((const char *) sqlite3_column_text(get_features_stmt,1));
  feat->count=sqlite3_column_int(get_features_stmt,2);
  HASH_ADD_KEYPTR( hh, feature_map, feat->label, strlen(feat->label), feat );
  load_ftr_by_feature_id(feat->id);
  return 0;
}

int load_ftr(){
  struct feature_test_result *result;
  int keylen;
  result=ck_alloc(sizeof(struct feature_test_result));
  
  result->id=sqlite3_column_int(get_ftr_by_feature_stmt,0);
  result->url_test_id=sqlite3_column_int(get_ftr_by_feature_stmt,1);
  result->feature_id=sqlite3_column_int(get_ftr_by_feature_stmt,2);
  result->code_200=sqlite3_column_int(get_ftr_by_feature_stmt,3);
  result->code_301=sqlite3_column_int(get_ftr_by_feature_stmt,4);
  result->code_302=sqlite3_column_int(get_ftr_by_feature_stmt,5);
  result->code_401=sqlite3_column_int(get_ftr_by_feature_stmt,6);
  result->code_403=sqlite3_column_int(get_ftr_by_feature_stmt,7);
  result->code_500=sqlite3_column_int(get_ftr_by_feature_stmt,8);
  result->code_other=sqlite3_column_int(get_ftr_by_feature_stmt,9);
  result->count=sqlite3_column_int(get_ftr_by_feature_stmt,10);
  keylen =   offsetof(struct feature_test_result, feature_id) + sizeof(int) - offsetof(struct feature_test_result, url_test_id);
  HASH_ADD( hh, result_map, url_test_id, keylen, result);
  return 0;
}


struct url_test *load_test(sqlite3_stmt *stmt){
    struct url_test *test;
    printf("load test\n");
    test=ck_alloc(sizeof(struct url_test));
    test->id=sqlite3_column_int(stmt,0);
    printf("url\n");
    test->url=(char *) strdup((const char *) sqlite3_column_text(stmt,1));
    printf("desc\n");
    test->description=(char *) strdup((const char *) sqlite3_column_text(stmt,2));
    test->code_200=sqlite3_column_int(stmt,3);
    test->code_301=sqlite3_column_int(stmt,4);
    test->code_302=sqlite3_column_int(stmt,5);
    test->code_401=sqlite3_column_int(stmt,6);
    test->code_403=sqlite3_column_int(stmt,7);
    test->code_500=sqlite3_column_int(stmt,8);
    test->code_other=sqlite3_column_int(stmt,9);
    test->count=sqlite3_column_int(stmt,10);
    test->flags=(unsigned int) sqlite3_column_int(stmt,11);
    HASH_ADD_INT( test_map, id, test );
    printf("fin test\n");
    return test;
}

void add_or_update_url(char *url, char *description, unsigned int flags){
  struct url_test *test;
  sqlite3_reset(get_test_by_url_stmt);
  sqlite3_bind_text(get_test_by_url_stmt,1,url,-1,SQLITE_STATIC);
  if(sqlite3_step(get_test_by_url_stmt) == SQLITE_ROW){
    test=load_test(get_test_by_url_stmt);
  }
  else{
    test=ck_alloc(sizeof(struct url_test));
    test->dirty=1;
    test->id=-1;
  }
  test->url=url;
  test->description=description;
  test->flags=flags;
  save_url_test(test);
}

int load_tests(){
  while (sqlite3_step(get_tests_stmt) == SQLITE_ROW){
    load_test(get_tests_stmt);
  }
  return 0;
}


