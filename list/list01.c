/**
 *  File name :     list01.c
 *  Comments :       list study in the Linux Kerenl
 *  Source from:    /linux/include/list.h
 *  Author :        Jung,JaeJoon (rgbi3307@nate.com) on the www.kernel.bz
 *  Creation:       2016-02-26
 *      GPL 라이센서에 따라서 위의 저자정보는 삭제하지 마시고 공유해 주시기 바랍니다.
  *          수정한 내용은 아래의  Edition란에 추가해 주세요.
 *  Edition :
 */

#include <stdio.h>

//include/linux/stddef.h
#define offsetof(TYPE, MEMBER) ((unsigned long) &((TYPE *)0)->MEMBER)

//include/linux/kernel.h
#define container_of(ptr, type, member) ({	\
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member) ); })

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static inline void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list;
	list->prev = list;
}

struct fox {
	unsigned long tail_length;
	unsigned long weight;
	int			  is_fantastic;
	struct list_head	list;
};

struct fox red_fox = {
	.tail_length = 40,
	.weight = 10,
	.is_fantastic = 0,
	.list = LIST_HEAD_INIT (red_fox.list),
	//INIT_LIST_HEAD (&red_fox->list);
};

int main (void)
{
	printf ("size: %d, %d, %d, %d\n",
        sizeof(struct fox), sizeof(unsigned long), sizeof(int), sizeof(struct list_head));

	printf ("red_fox value: %d, %d, %d\n"
        , red_fox.tail_length, red_fox.weight, red_fox.is_fantastic);

	printf ("red_fox addr: 0x%X, 0x%X, 0x%X, 0x%X, 0x%X\n"
        , &red_fox, &red_fox.tail_length, &red_fox.weight, &red_fox.is_fantastic, &red_fox.list);

	printf ("offset: %d, %d, %d, %d\n", offsetof(struct fox, tail_length)
									, offsetof(struct fox, weight)
									, offsetof(struct fox, is_fantastic)
									, offsetof(struct fox, list));

	printf ("container_of: 0x%X, 0x%X\n", container_of(&red_fox.weight, struct fox, weight)
									, container_of(&red_fox.list, struct fox, list) );
	return 0;
}
