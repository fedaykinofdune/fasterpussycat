CREATE TABLE "aho_corasick_feature_triggers" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "feature_id" INTEGER NOT NULL, trigger VARCHAR(50));
CREATE TABLE "feature_test_results" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "count" INTEGER DEFAULT 0, "code_200" INTEGER DEFAULT 0, "code_301" INTEGER DEFAULT 0, "code_302" INTEGER DEFAULT 0, "code_401" INTEGER DEFAULT 0, "code_403" INTEGER DEFAULT 0, "code_500" INTEGER DEFAULT 0, "code_other" INTEGER DEFAULT 0, "url_test_id" INTEGER NOT NULL, "feature_id" INTEGER NOT NULL);
CREATE TABLE "features" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "label" VARCHAR(50), "count" INTEGER DEFAULT 0);
CREATE TABLE "url_tests" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "url" VARCHAR(50), "count" INTEGER DEFAULT 0, "code_200" INTEGER DEFAULT 0, "code_301" INTEGER DEFAULT 0, "code_302" INTEGER DEFAULT 0, "code_401" INTEGER DEFAULT 0, "code_403" INTEGER DEFAULT 0, "code_500" INTEGER DEFAULT 0, "code_other" INTEGER DEFAULT 0, "description" VARCHAR(50), "flags" INTEGER DEFAULT 0);
CREATE INDEX "index_feature_test_results_feature" ON "feature_test_results" ("feature_id");
CREATE UNIQUE INDEX "index_feature_test_results_feature_url" ON "feature_test_results" ("url_test_id", "feature_id");
CREATE INDEX "index_feature_test_results_url_test" ON "feature_test_results" ("url_test_id");
CREATE UNIQUE INDEX "index_trigger" ON "aho_corasick_feature_triggers" ("trigger");
