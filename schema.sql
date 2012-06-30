CREATE TABLE "aho_corasick_feature_triggers" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "feature_id" INTEGER NOT NULL, trigger VARCHAR(50));
CREATE TABLE "feature_test_results" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "count" INTEGER DEFAULT 0 NOT NULL, "success" INTEGER DEFAULT 0 NOT NULL, "url_test_id" INTEGER NOT NULL, "feature_id" INTEGER NOT NULL);
CREATE TABLE "features" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "label" VARCHAR(50), "count" INTEGER DEFAULT 0);

CREATE TABLE "feature_selections" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "feature_id" INTEGER NOT NULL, "url_test_id" INTEGER DEFAULT 0);
CREATE TABLE "url_tests" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "url" VARCHAR(50), "count" INTEGER DEFAULT 0 NOT NULL, "success" INTEGER DEFAULT 0 NOT NULL,  "flags" INTEGER DEFAULT 0 NOT NULL);
CREATE TABLE "dir_links" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "child_success" INTEGER DEFAULT 0 NOT NULL, "child_tested" INTEGER DEFAULT 0 NOT NULL, "parent_success" INTEGER DEFAULT 0 NOT NULL, "parent_count" INTEGER DEFAULT 0 NOT NULL  );
CREATE UNIQUE INDEX "index_feature_test_results_feature_url" ON "feature_test_results" ("feature_id","url_test_id");
CREATE UNIQUE INDEX "index_feature_label" ON "features" ("label");
CREATE UNIQUE INDEX "index_trigger" ON "aho_corasick_feature_triggers" ("trigger");
CREATE UNIQUE INDEX "index_url" ON "url_tests" ("url");
CREATE INDEX "index_feature_selections_url" ON "feature_selections" ("url_test_id");
