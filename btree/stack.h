//
//  Source: stack.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-03-18 스택 헤더파일 정의
//		yyyy-mm-dd ...
//

#include "dtype.h"

//스택 함수들 선언 ------------------------------------------------------------
//스택 생성
STACK* stack_create (void);
//스택 삭제
int stack_destroy (STACK* stack);
//top에서 스택 삭제
int stack_drop_from_top (STACK* stack, int flag);
//bottom에서 스택 삭제
int stack_drop_from_bottom (STACK* stack, int flag);

//스택의 top에 push한다.
bool stack_push_top (STACK* stack, void* data_in);
//스택의 bottom에 push한다.
bool stack_push_bottom (STACK* stack, void* data_in);

//스택 높이(height)에 제한을 두어 top에 push한다.
bool stack_push_limit (STACK* stack, void* data_in, int height);
//스택이 height 높이 만큼 되도록 bottom에서 제거한다.
void stack_remove_bottom (STACK* stack, int height);

//스택 상단에서 데이터 가져옴
void* stack_pop_top (STACK* stack, int flag);
void* stack_top (STACK* stack, int flag);

//스택 하단에서 데이터 가져옴
void* stack_pop_bottom (STACK* stack, int flag);
void* stack_bottom (STACK* stack, int flag);

bool stack_is_empty (STACK* stack);
bool stack_is_full (STACK* stack);
int stack_count (STACK* stack);
