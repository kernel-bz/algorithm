//
//  Source: umem.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-01-25 메모리 관련 함수들을 작성하다.
//		yyyy-mm-dd ...
//

//#include <malloc.h>

#include "dtype.h"

#ifdef __LINUX

#include <malloc.h>

/*
struct mallinfo {	  // all sizes in bytes
        int arena;    // size of data segment used by malloc
        int ordblks;  // number of free chunks
        int smblks;   // number of fast bins
        int hblks;    // number of anonymous mappings
        int hblkhd;   // size of anonymous mappings
        int usmblks;  // maximum total allocated size
        int fsmblks;  // size of available fast bins
        int uordblks; // size of total allocated space
        int fordblks; // size of available chunks
        int keepcost; // size of trimmable space
};
*/

void mem_info (void)
{
	struct mallinfo m;

	//malloc_stats ();
	m = mallinfo ();
	printf ("** Used Memory: %d KBytes\n", m.arena / 1024);
	//printf ("** Free Memory: %d KBytes\n", m.ordblks / 1024);
}

#endif
