/* bayes.c */
void setup_bayes(void);
double positive_predictive_value(struct url_test *test, struct feature *f);
double negative_predictive_value(struct url_test *test, struct feature *f);
double information(double prob);
double information_gain(struct test_score *score, struct feature *f);

double expected_change(struct test_score *score, struct feature *f);

double expected_change_ratio(struct test_score *score, struct feature *f);
double calculate_new_posterior(double posterior, unsigned int hypothesis_true, unsigned int sample_size, unsigned int evidence_and_hypothesis_true, unsigned int evidence_count, int evidence_occurred);
void show_feature_predictive_values(void);
double get_test_probability(struct url_test *test, struct target *t);
