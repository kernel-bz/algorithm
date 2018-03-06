//
//	file name 	: queue01.c
//	author 		: Jung,JaeJoon(rgbi3307@nate.com) on the kernel.bz
//	comments	: Linux Kernel queue test
//
#include <stdio.h>
#include "kfifo.h"

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1 << PAGE_SHIFT)	//4KB
#define GFP_KERNEL	0

int main()
{
	struct kfifo kfifo;
	int ret;

	ret = kfifo_alloc(&kfifo, PAGE_SIZE, GFP_KERNEL);
	if (ret) return ret;	//error

	unsigned int i, val;
	for (i = 0; i < 32; i++)
		kfifo_in(kfifo, &i, sizeof(i));

	ret = kfifo_out_peek(kfifo, &val, sizeof(val), 0);
	if (ret != sizeof(val))	return -1;	//error

	printf("val=%u\n", val);

	return 0;
}

