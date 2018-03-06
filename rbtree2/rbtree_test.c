/**
 *  File name :     rbtree_test.c
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

#define NodeCnt     8


struct rb_test {
	u32 key;
	struct rb_node rb;

	///following fields used for testing augmented rbtree functionality
	u32 val;
	u32 augmented;
};

static struct rb_root root = RB_ROOT;
static struct rb_test Nodes[NodeCnt];


static void rbtree_insert(struct rb_test *node, struct rb_root *root)
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

    if (level > 3)
        printf("search level=%d\n",level);

	rb_link_node(&node->rb, parent, new);
	rb_insert_color(&node->rb, root);
}

static inline void rbtree_erase(struct rb_test *node, struct rb_root *root)
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

static void rbtree_insert_augmented(struct rb_test *node, struct rb_root *root)
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

static void rbtree_erase_augmented(struct rb_test *node, struct rb_root *root)
{
	rb_erase_augmented(&node->rb, root, &augment_callbacks);
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

static void rbtree_search_postorder_foreach(int nr_nodes)
{
	struct rb_test *cur, *n;
	int count = 0;
	rbtree_postorder_for_each_entry_safe(cur, n, &root, rb)
		count++;

	///WARN_ON_ONCE(count != nr_nodes);
}

static void rbtree_search_postorder(int nr_nodes)
{
	struct rb_node *rb;
	int count = 0;
	for (rb = rb_first_postorder(&root); rb; rb = rb_next_postorder(rb))
		count++;

	///WARN_ON_ONCE(count != nr_nodes);
}

static void rbtree_search(int nr_nodes)
{
	struct rb_node *rb;
	int count = 0, blacks = 0;
	u32 prev_key = 0;

	printf("rbtree_search ----------------------------------\n");

	for (rb = rb_first(&root); rb; rb = rb_next(rb)) {
		struct rb_test *node = rb_entry(rb, struct rb_test, rb);

        /**
		WARN_ON_ONCE(node->key < prev_key);
		WARN_ON_ONCE(is_red(rb) && (!rb_parent(rb) || is_red(rb_parent(rb))));
		if (!count)
			blacks = black_path_count(rb);
		else
			WARN_ON_ONCE((!rb->rb_left || !rb->rb_right) && blacks != black_path_count(rb));
		*/

		prev_key = node->key;
		printf("%d: 0x%X: %d\n", count, &node->rb, prev_key);
		count++;
	}
	printf("\n");

	///WARN_ON_ONCE(count != nr_nodes);
	///WARN_ON_ONCE(count < (1 << black_path_count(rb_last(&root))) - 1);

	rbtree_search_postorder(nr_nodes);
	rbtree_search_postorder_foreach(nr_nodes);
}

static void rbtree_search_augmented(int nr_nodes)
{
	struct rb_node *rb;

	rbtree_search(nr_nodes);
	for (rb = rb_first(&root); rb; rb = rb_next(rb)) {
		struct rb_test *node = rb_entry(rb, struct rb_test, rb);
		///WARN_ON_ONCE(node->augmented != augment_recompute(node));
	}
}

static void rbtree_nodes_init(int idx)
{
    int keys[3][8] = { { 10, 20, 30, 40, 50, 60, 70, 80 },
                       { 80, 70, 60, 50, 40, 30, 20, 10 },
                       { 10, 30, 20, 50, 40, 25, 22, 35 } };
	int i;

    printf("rbtree_nodes_init(%d) ==============================\n", idx);
	for (i = 0; i < NodeCnt; i++) {
		Nodes[i].key = keys[idx][i];
		printf("%d: 0x%X: %d\n", i, &Nodes[i].rb, keys[idx][i]);
	}
}

static int rbtree_test(void)
{
	int j;
	struct timeval time1, time2, time3;

    printf("rbtree insert testing...wait...\n");
    gettimeofday(&time1);

    for (j = 0; j < NodeCnt; j++)
		rbtree_insert(Nodes + j, &root);

    rbtree_search(NodeCnt);

    printf("rbtree erase testing...wait...\n");

	for (j = 0; j < NodeCnt; j++)
		rbtree_erase(Nodes + j, &root);

    rbtree_search(0);

	gettimeofday(&time2);
	time3.tv_sec = time2.tv_sec - time1.tv_sec;
	time3.tv_usec = time2.tv_usec - time1.tv_usec;
	printf("*runtime: %d sec %d usec\n\n", time3.tv_sec, time3.tv_usec);

	return 0;
}


static int rbtree_augmented_test(void)
{
	int j;
	struct timeval time1, time2, time3;

	printf("augmented rbtree insert testing...wait...\n");
	gettimeofday(&time1);

    for (j = 0; j < NodeCnt; j++)
        rbtree_insert_augmented(Nodes + j, &root);

    rbtree_search(NodeCnt);

    printf("augmented rbtree erase testing...wait...\n");

    for (j = 0; j < NodeCnt; j++)
        rbtree_erase_augmented(Nodes + j, &root);

    rbtree_search(0);

	gettimeofday(&time2);
	time3.tv_sec = time2.tv_sec - time1.tv_sec;
	time3.tv_usec = time2.tv_usec - time1.tv_usec;
	printf("*runtime: %d sec %d usec\n\n", time3.tv_sec, time3.tv_usec);

	return 0;
}

int main(void)
{
    int i;

    printf("datatype size: u16=%d, u32=%d, u64=%d\n",
        sizeof(u16), sizeof(u32), sizeof(u64));

    printf("Nodes Size: %d / %d, Count: %d\n",
        sizeof(Nodes), sizeof(Nodes[0]), sizeof(Nodes)/sizeof(Nodes[0]));

    for (i=0; i<3; i++) {
        rbtree_nodes_init(i);
        rbtree_test();
        ///rbtree_augmented_test();
    }
    printf("test end.\n");

    return 0;
}

