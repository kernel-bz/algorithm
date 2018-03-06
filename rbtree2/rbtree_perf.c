/**
 *  File name :     rbtree_perf.c
 *  Comments :       rbtree study in the Linux Kerenl
 *  Source from:    /lib/rbtree_test.c
 *  Author :        Jung,JaeJoon (rgbi3307@nate.com)
 *  Creation:       2016-02-26
 *      GPL 라이센서에 따라서 위의 저자정보는 삭제하지 마시고 공유해 주시기 바랍니다.
  *          수정한 내용은 아래의  Edition란에 추가해 주세요.
 *  Edition :
 */

 #include <stdio.h>
 #include <stdlib.h>
 #include <time.h>

//#include <linux/module.h>
//#include <asm/timex.h>
#include "rbtree_augmented.h"
#include "random.h"

#define NODES       100
#define PERF_LOOPS  100000
#define CHECK_LOOPS 100

///include/asm-generic/timex.h
static inline cycles_t get_cycles(void)
{
    return 0;
}


struct rb_test {
	u32 key;
	struct rb_node rb;

	///following fields used for testing augmented rbtree functionality
	u32 val;
	u32 augmented;
};

static struct rb_root root = RB_ROOT;
static struct rb_test nodes[NODES];

static struct rnd_state rnd;

static void insert(struct rb_test *node, struct rb_root *root)
{
	struct rb_node **new = &root->rb_node, *parent = NULL;
	u32 key = node->key;
	unsigned int level = 0;

	while (*new) {
		parent = *new;
		level++;
		///container_of(ptr, type, member)
		if (key < rb_entry(parent, struct rb_test, rb)->key)
			new = &parent->rb_left;
		else
			new = &parent->rb_right;
	}

    if (level > 8)
        printf("search level=%d\n",level);

	rb_link_node(&node->rb, parent, new);
	rb_insert_color(&node->rb, root);
}

static inline void erase(struct rb_test *node, struct rb_root *root)
{
	rb_erase(&node->rb, root);
}

static inline u32 augment_recompute(struct rb_test *node)
{
	u32 max = node->val, child_augmented;
	if (node->rb.rb_left) {
		child_augmented = rb_entry(node->rb.rb_left, struct rb_test,
					   rb)->augmented;
		if (max < child_augmented)
			max = child_augmented;
	}
	if (node->rb.rb_right) {
		child_augmented = rb_entry(node->rb.rb_right, struct rb_test,
					   rb)->augmented;
		if (max < child_augmented)
			max = child_augmented;
	}
	return max;
}

RB_DECLARE_CALLBACKS(static, augment_callbacks, struct rb_test, rb,
		     u32, augmented, augment_recompute)

static void insert_augmented(struct rb_test *node, struct rb_root *root)
{
	struct rb_node **new = &root->rb_node, *rb_parent = NULL;
	u32 key = node->key;
	u32 val = node->val;
	struct rb_test *parent;

	while (*new) {
		rb_parent = *new;
		parent = rb_entry(rb_parent, struct rb_test, rb);
		if (parent->augmented < val)
			parent->augmented = val;
		if (key < parent->key)
			new = &parent->rb.rb_left;
		else
			new = &parent->rb.rb_right;
	}

	node->augmented = val;
	rb_link_node(&node->rb, rb_parent, new);
	rb_insert_augmented(&node->rb, root, &augment_callbacks);
}

static void erase_augmented(struct rb_test *node, struct rb_root *root)
{
	rb_erase_augmented(&node->rb, root, &augment_callbacks);
}

static void init(void)
{
	int i;
	for (i = 0; i < NODES; i++) {
		nodes[i].key = prandom_u32_state(&rnd);
		nodes[i].val = prandom_u32_state(&rnd);
	}
}

static bool is_red(struct rb_node *rb)
{
	return !(rb->__rb_parent_color & 1);
}

static int black_path_count(struct rb_node *rb)
{
	int count;
	for (count = 0; rb; rb = rb_parent(rb))
		count += !is_red(rb);
	return count;
}

static void check_postorder_foreach(int nr_nodes)
{
	struct rb_test *cur, *n;
	int count = 0;
	rbtree_postorder_for_each_entry_safe(cur, n, &root, rb)
		count++;

	///WARN_ON_ONCE(count != nr_nodes);
}

static void check_postorder(int nr_nodes)
{
	struct rb_node *rb;
	int count = 0;
	for (rb = rb_first_postorder(&root); rb; rb = rb_next_postorder(rb))
		count++;

	///WARN_ON_ONCE(count != nr_nodes);
}

static void check(int nr_nodes)
{
	struct rb_node *rb;
	int count = 0, blacks = 0;
	u32 prev_key = 0;

	///printf("*check ----------------------------------\n");

	for (rb = rb_first(&root); rb; rb = rb_next(rb)) {
		struct rb_test *node = rb_entry(rb, struct rb_test, rb);
		///WARN_ON_ONCE(node->key < prev_key);
		///WARN_ON_ONCE(is_red(rb) && (!rb_parent(rb) || is_red(rb_parent(rb))));
		if (!count)
			blacks = black_path_count(rb);
		else
			///WARN_ON_ONCE((!rb->rb_left || !rb->rb_right) && blacks != black_path_count(rb));
		prev_key = node->key;
		count++;
		///printf("%d:%d, ", count, prev_key);
	}
	///printf("\n");

	///WARN_ON_ONCE(count != nr_nodes);
	///WARN_ON_ONCE(count < (1 << black_path_count(rb_last(&root))) - 1);

	check_postorder(nr_nodes);
	check_postorder_foreach(nr_nodes);
}

static void check_augmented(int nr_nodes)
{
	struct rb_node *rb;

	check(nr_nodes);
	for (rb = rb_first(&root); rb; rb = rb_next(rb)) {
		struct rb_test *node = rb_entry(rb, struct rb_test, rb);
		///WARN_ON_ONCE(node->augmented != augment_recompute(node));
	}
}

static int rbtree_test_init(void)
{
	int i, j;
	//cycles_t time1, time2, time;
	time_t time1, time2, time3;  ///long int

    printf("datatype size: u16=%d, u32=%d, u64=%d, time_t=%d\n",
        sizeof(u16), sizeof(u32), sizeof(u64), sizeof(time_t));

    printf("nodes size: %d / %d\n", sizeof(nodes[0]), sizeof(nodes));

    printf("random number initialize...\n");
	prandom_seed_state(&rnd, 3141592653589793238ULL);
	printf("rnd: %d, %d, %d, %d\n", rnd.s1, rnd.s2, rnd.s3, rnd.s4);

	init();

	printf("rbtree testing...wait...\n");

	//time1 = get_cycles();
	time(&time1);

/**
	nodes[0].key = 10;
	nodes[1].key = 30;
	nodes[2].key = 20;
	nodes[3].key = 50;
	nodes[4].key = 40;
	nodes[5].key = 25;
	nodes[6].key = 22;
	nodes[7].key = 35;
*/
	nodes[0].key = 10;
	nodes[1].key = 20;
	nodes[2].key = 30;
	nodes[3].key = 40;
	nodes[4].key = 50;
	nodes[5].key = 60;
	nodes[6].key = 70;
	nodes[7].key = 80;

	for (i = 0; i < PERF_LOOPS; i++) {
		for (j = 0; j < NODES; j++)
			insert(nodes + j, &root);
		for (j = 0; j < NODES; j++)
			erase(nodes + j, &root);
	}

	//time2 = get_cycles();
	time(&time2);
	time3 = time2 - time1;

	///time3 = div_u64(time, PERF_LOOPS);
	///time3 = time3 / PERF_LOOPS;
	///printf(" -> %llu cycles\n", (unsigned long long)time3);
	printf("runtime: %d seconds\n", time3);

	for (i = 0; i < CHECK_LOOPS; i++) {
		init();
		for (j = 0; j < NODES; j++) {
			check(j);
			insert(nodes + j, &root);
		}
		for (j = 0; j < NODES; j++) {
			check(NODES - j);
			erase(nodes + j, &root);
		}
		check(0);
	}

	printf("augmented rbtree testing...wait...\n");

	init();

	//time1 = get_cycles();
	time(&time1);

	for (i = 0; i < PERF_LOOPS; i++) {
		for (j = 0; j < NODES; j++)
			insert_augmented(nodes + j, &root);
		for (j = 0; j < NODES; j++)
			erase_augmented(nodes + j, &root);
	}

	//time2 = get_cycles();
	time(&time2);
	time3 = time2 - time1;

	//time3 = div_u64(time, PERF_LOOPS);
	//time3 = time3 / PERF_LOOPS;
	//printf(" -> %llu cycles\n", (unsigned long long)time3);
	printf("runtime: %d seconds\n", time3);

	for (i = 0; i < CHECK_LOOPS; i++) {
		init();
		for (j = 0; j < NODES; j++) {
			check_augmented(j);
			insert_augmented(nodes + j, &root);
		}
		for (j = 0; j < NODES; j++) {
			check_augmented(NODES - j);
			erase_augmented(nodes + j, &root);
		}
		check_augmented(0);
	}

	///return -EAGAIN; /* Fail will directly unload the module */
	return -11;
}

static void rbtree_test_exit(void)
{
	printf("test exit.\n");
}


int main(void)
{
    rbtree_test_init();

    rbtree_test_exit();

    return 0;
}

