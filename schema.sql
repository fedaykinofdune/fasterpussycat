CREATE TABLE "aho_corasick_feature_triggers" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "feature_id" INTEGER NOT NULL, trigger VARCHAR(50));
CREATE TABLE "dir_links" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "parent_id" INTEGER NOT NULL, "child_id" INTEGER  NOT NULL, "count" INTEGER DEFAULT 0 NOT NULL, "parent_success" INTEGER DEFAULT 0 NOT NULL, "child_success" INTEGER DEFAULT 0 NOT NULL, "parent_child_success" INTEGER DEFAULT 0 NOT NULL  );
CREATE TABLE "feature_selections" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "feature_id" INTEGER NOT NULL, "url_test_id" INTEGER DEFAULT 0);
CREATE TABLE "feature_test_results" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "count" INTEGER DEFAULT 0 NOT NULL, "success" INTEGER DEFAULT 0 NOT NULL, "url_test_id" INTEGER NOT NULL, "feature_id" INTEGER NOT NULL);
CREATE TABLE "features" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "label" VARCHAR(50), "count" INTEGER DEFAULT 0);
CREATE TABLE "url_tests" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "url" VARCHAR(50), "count" INTEGER DEFAULT 0 NOT NULL, "success" INTEGER DEFAULT 0 NOT NULL,  "flags" INTEGER DEFAULT 0 NOT NULL);
CREATE UNIQUE INDEX "dir_links_idx" ON "dir_links" ("parent_id","child_id");
CREATE UNIQUE INDEX "index_feature_label" ON "features" ("label");
CREATE INDEX "index_feature_selections_url" ON "feature_selections" ("url_test_id");
CREATE UNIQUE INDEX "index_feature_test_results_feature_url" ON "feature_test_results" ("feature_id","url_test_id");
CREATE UNIQUE INDEX "index_trigger" ON "aho_corasick_feature_triggers" ("trigger");
CREATE UNIQUE INDEX "index_url" ON "url_tests" ("url");
CREATE TABLE "results" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "time" INTEGER NOT NULL, "url" VARCHAR NOT NULL, "code" INTEGER NOT NULL, "mime" VARCHAR NOT NULL, "flags" INTEGER NOT NULL, "content_length" INTEGER NOT NULL);
CREATE TABLE "results_post" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "result_id" INTEGER NOT NULL, "key" VARCHAR NOT NULL, "value" VARCHAR);
CREATE UNIQUE INDEX "index_results_url" ON "results" ("url");

