//
//	file name	: rb01.c
//	author		: Jung,JaeJoon(rgbi3307@nate.com) on the www.kernel.bz
//	comments	: kernel red-black tree test
//				  kernel version 2.6.31
//
#include <stdio.h>
#include <stdlib.h>
#include "rbtree.h"
//rbtree.c

struct my_struct {
	int value;
	struct rb_node node;
};

//struct rb_root my_rb_root = RB_ROOT;

struct my_struct *my_rb_search(struct rb_root *root, int value)
{
	struct rb_node *node = root->rb_node;  //top of the tree

	while (node) {
		struct my_struct *this = rb_entry(node, struct my_struct, node);	//container_of

	    if (this->value > value)
			node = node->rb_left;
	    else if (this->value < value)
			node = node->rb_right;
	    else
			return this;  //found it
  	}
	return NULL;
}

void my_rb_insert(struct rb_root *root, struct my_struct *new)
{
	struct rb_node **link = &root->rb_node, *parent = NULL;
	int value = new->value;

	//go to the bottom of the tree
	while (*link) {
	    parent = *link;
	    struct my_struct *this = rb_entry(parent, struct my_struct, node);

	    if (this->value > value)
			link = &(*link)->rb_left;
	    else
			link = &(*link)->rb_right;
	}

	//put the new node there
	rb_link_node(&(new->node), parent, link);
	rb_insert_color(&(new->node), root);
}

void my_rb_output(struct rb_root *my_rb_root)
{
	struct rb_node *node;

	printf("rb_node: ");
	for (node = rb_first(my_rb_root); node; node = rb_next(node))
		printf("%d, ", rb_entry(node, struct my_struct, node)->value);
	printf("\n");
}

void my_rb_drop(struct rb_root *my_rb_root)
{
	struct rb_node *node;

	printf("rb_drop: ");
	for (node = rb_first(my_rb_root); node; node = rb_next(node))
		free(rb_entry(node, struct my_struct, node));
	my_rb_root->rb_node = NULL;
	printf("\n");
}

int main(void)
{
	struct rb_root my_rb_root = RB_ROOT;
	struct my_struct *new;
	struct rb_node *node;
	int i;

	//insert to rbtree
	for (i = 0; i < 20; i++) {
		new = malloc(sizeof(struct my_struct));
		new->value = i%8;
		my_rb_insert(&my_rb_root, new);
	}
	//output from rbtree
	my_rb_output(&my_rb_root);

	new = my_rb_search(&my_rb_root, 0);
	if (new) {
		printf ("value=%d, ", new->value);
		rb_erase(&new->node, &my_rb_root);
		free(new);
		printf("rb_erase.\n");
	}
	else printf ("Cannot find!\n");

	my_rb_output(&my_rb_root);
	my_rb_drop(&my_rb_root);
	my_rb_output(&my_rb_root);

	//insert to rbtree
	for (i = 0; i < 20; i++) {
		new = malloc(sizeof(struct my_struct));
		new->value = i%8;
		my_rb_insert(&my_rb_root, new);
	}
	//output from rbtree
	my_rb_output(&my_rb_root);

	return 0;
}

