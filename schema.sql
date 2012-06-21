CREATE TABLE "aho_corasick_feature_triggers" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "feature_id" INTEGER NOT NULL, trigger VARCHAR(50));
CREATE TABLE "feature_test_results" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "count" INTEGER DEFAULT 0, "code_200" INTEGER DEFAULT 0, "code_reloc" INTEGER DEFAULT 0, "code_401" INTEGER DEFAULT 0, "code_403" INTEGER DEFAULT 0, "code_err" INTEGER DEFAULT 0, "url_test_id" INTEGER NOT NULL, "feature_id" INTEGER NOT NULL);
CREATE TABLE "features" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "label" VARCHAR(50), "count" INTEGER DEFAULT 0);
CREATE TABLE "url_tests" ("id" INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "url" VARCHAR(50), "count" INTEGER DEFAULT 0, "code_200" INTEGER DEFAULT 0, "code_reloc" INTEGER DEFAULT 0, "code_401" INTEGER DEFAULT 0, "code_403" INTEGER DEFAULT 0, "code_err" INTEGER DEFAULT 0, "flags" INTEGER DEFAULT 0);
CREATE UNIQUE INDEX "index_feature_test_results_feature_url" ON "feature_test_results" ("feature_id","url_test_id");
CREATE UNIQUE INDEX "index_url_test_url" ON "url_test" ("url");
CREATE UNIQUE INDEX "index_feature_label" ON "features" ("label");
CREATE INDEX "index_feature_test_results_url_test" ON "feature_test_results" ("feature_id", "url_test_id");
CREATE UNIQUE INDEX "index_trigger" ON "aho_corasick_feature_triggers" ("trigger");
