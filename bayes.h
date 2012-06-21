/* bayes.c */
void setup_bayes(void);
void calculate_new_posterior(mpq_t *posterior, struct url_test *test, struct feature_test_result *ftr);
double get_test_probability(struct url_test *test, struct target *t);
