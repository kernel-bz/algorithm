/**
 *  File name :     list02.c
 *  Comments :       list study in the Linux Kerenl
 *  Source from:    /linux/include/list.h
 *  Author :        Jung,JaeJoon (rgbi3307@nate.com)
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

struct fox red_fox = {
	.name = "red_fox",
	.tail_length = 40,
	.weight = 10,
	.is_fantastic = 0,
	.list = LIST_HEAD_INIT (red_fox.list),
	//INIT_LIST_HEAD (&red_fox->list);
};

struct fox white_fox = {
	.name = "white_fox",
	.tail_length = 50,
	.weight = 20,
	.is_fantastic = 1,
	.list = LIST_HEAD_INIT (white_fox.list),
};
//list head define and init
static LIST_HEAD (fox_list);

int main (void)
{
	struct list_head *p;
	struct fox *f;

	list_add(&red_fox.list, &fox_list);
	list_add(&white_fox.list, &fox_list);

	f = malloc(sizeof(*f));
	strcpy(f->name,  "black_fox");
	f->tail_length = 30;
	f->weight = 5;
	f->is_fantastic = 0;
	//f->list = LIST_HEAD_INIT (f->list);

	list_add_tail(&f->list, &fox_list);

	__list_for_each(p, &fox_list) {
		f = list_entry(p, struct fox, list);
		printf ("fox value: %s, %d, %d, %d\n", f->name
										, f->tail_length
										, f->weight
										, f->is_fantastic);
		printf ("\n");
	}
	return 0;
}

