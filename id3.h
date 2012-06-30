#define ID3_ROOT 3
#define ID3_NO_EVD 0
#define ID3_EVD 1
struct id3_node;

struct id3_node {
  struct feature *feature;
  struct id3_node *child_evidence;
  struct id3_node *child_no_evidence;
  struct id3_node *parent;
  struct test_score *scores;
  unsigned char type;
};
/* id3.c */
int in_chain(struct feature *f, struct id3_node *root);
void print_path(struct id3_node *node);
struct feature *pick_feature(struct id3_node *root);
struct id3_node *spawn_child(struct id3_node *parent, int evidence);
struct id3_node *build_tree(struct id3_node *root);
