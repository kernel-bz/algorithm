/**
 *  File name :     list02.c
 *  Comments :       list study in the Linux Kerenl
 *  Source from:    /linux/include/list.h
 *  Author :        Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *  Creation:       2016-02-26
 *      GPL 라이센서에 따라서 위의 저자정보는 삭제하지 마시고 공유해 주시기 바랍니다.
  *          수정한 내용은 아래의  Edition란에 추가해 주세요.
 *  Edition :
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

struct fox {
	char				name[20];
	unsigned long 		tail_length;
	unsigned long 		weight;
	int			  		is_fantastic;
	struct list_head	list;
};

//list head define and init
static LIST_HEAD (ListHeadFox);

struct fox* fox_alloc(char *name, unsigned long tail, unsigned long weight, int isf)
{
	struct fox* fox;

	fox = malloc(sizeof(*fox));
	if (!fox) return NULL;

	strcpy(fox->name, name);
	fox->tail_length = tail;
	fox->weight = weight;
	fox->is_fantastic = isf;

	//INIT_LIST_HEAD(&fox->list);

	return fox;
}

void list_output(struct list_head *head)
{
	struct list_head *list_head;
	struct fox* fox;

	printf ("-- list output --\n");
	__list_for_each(list_head, head) {
		fox = list_entry(list_head, struct fox, list);
		printf ("fox value: %s, %d, %d, %d\n", fox->name
										, fox->tail_length
										, fox->weight
										, fox->is_fantastic);
	}
}

int main (void)
{
	LIST_HEAD (list_head_fox2);
	LIST_HEAD (list_head_fox3);
	struct list_head *list_head_tmp;
	struct fox* fox;

	fox = fox_alloc("red_fox", 30, 20, 0);
	if (!fox) return -1;
	list_add(&fox->list, &ListHeadFox);	//add to ListHeadFox

	fox = fox_alloc("white_fox", 40, 30, 1);
	if (!fox) return -1;
	list_add(&fox->list, &ListHeadFox);	//add to ListHeadFox

	fox = fox_alloc("black_fox", 20, 10, 2);
	if (!fox) return -1;
	list_add(&fox->list, &list_head_fox2);	//add to list_head_fox2

	fox = fox_alloc("yellow_fox", 15, 10, 3);
	if (!fox) return -1;
	list_add(&fox->list, &list_head_fox2);	//add to list_head_fox2

	list_output(&ListHeadFox);
	list_output(&list_head_fox2);

	list_splice(&list_head_fox2, &ListHeadFox);	//merge list_head_fox2 to ListHeadFox

	list_output(&ListHeadFox);
	//list_output(&list_head_fox2);

	//cut to list_head_fox3 from ListHeadFox
	list_cut_position(&list_head_fox3, &ListHeadFox, (&ListHeadFox)->next->next);
	list_output(&ListHeadFox);
	list_output(&list_head_fox3);

	return 0;
}
