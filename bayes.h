/* bayes.c */
void setup_bayes(void);
unsigned int get_test_successes(struct url_test *test, unsigned int codes);
unsigned int get_ftr_successes(struct feature_test_result *test, unsigned int codes);
void calculate_new_posterior(mpq_t *posterior, struct url_test *test, struct feature_test_result *ftr, unsigned int codes);
double get_test_probability(struct url_test *test, struct target *t);
