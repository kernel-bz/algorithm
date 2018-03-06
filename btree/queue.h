//
//  Source: queue.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-03-19 큐 헤더파일 정의
//		yyyy-mm-dd ...
//

#include "dtype.h"

QUEUE* que_create (void);
QUEUE* que_destroy (QUEUE*	queue);

bool que_dequeue (QUEUE* queue, void** data_out);
bool que_enqueue (QUEUE* queue, void*  data_in);
bool que_front (QUEUE* queue, void** data_out);
bool que_rear (QUEUE* queue, void** data_out);
int  que_count (QUEUE* queue);

bool que_is_empty (QUEUE* queue);
bool que_is_full  (QUEUE* queue);
