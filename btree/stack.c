//
//  Source: stack.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-03-17 스택 자료구조를 코딩하다.
//		2010-03-29 스택을 양방향(top,bottom)으로 작업할 수 있도록 수정하다.
//		2011-02-11 스택 할당과 해제 방향(top,bottom)을 수정하다.
//		yyyy-mm-dd ...
//

#include <stdlib.h>	//malloc
#include "stack.h"

//스택 생성(스택 구조체 메모리 할당)
STACK* stack_create (void)
{
	STACK* stack;

	stack = (STACK*)malloc (sizeof(STACK));
	if (stack) {
		stack->top = NULL;
		stack->bottom = NULL;		
		stack->count = 0;
	}
	return stack;
}

//스택 삭제
int stack_destroy (STACK* stack)
{
	NODE2* node_temp;
	int cnt = 0;

	if (stack) {
		while (stack->count > 0) {
			free (stack->top->data);	//데이터 메모리 해제
			node_temp = stack->top;
			stack->top = stack->top->next;	//top next link
			free (node_temp);
			(stack->count)--;
		}
		cnt = stack->count;
		free (stack);	//스택 메모리 해제
	}
	return cnt;	//0: 스택 모두 삭제 
}

//top에서 스택 삭제
int stack_drop_from_top (STACK* stack, int flag)
{
	NODE2* node_temp;
	int cnt = 0;

	if (stack) {
		//while (stack->top) {
		while (stack->count > 0) {
			free (stack->top->data);	//데이터 메모리 해제
			node_temp = stack->top;
			stack->top = stack->top->next;	//top next link
			free (node_temp);
			(stack->count)--;
		}
		cnt = stack->count;
		if (flag) free (stack);	//스택 메모리 해제
	}
	return cnt;	//0: 스택 모두 삭제 
}

//bottom에서 스택 삭제
int stack_drop_from_bottom (STACK* stack, int flag)
{
	NODE2* node_temp;
	int cnt = 0;

	if (stack) {
		//while (stack->bottom) {
		while (stack->count > 0) {
			free (stack->bottom->data);	//데이터 메모리 해제
			node_temp = stack->bottom;
			stack->bottom = stack->bottom->next;	//bottom next link
			free (node_temp);
			(stack->count)--;
		}
		cnt = stack->count;
		if (flag) free (stack);	//스택 메모리 해제
	}
	return cnt;	//0: 스택 모두 삭제 
}

//스택의 top에 push한다.
bool stack_push_top (STACK* stack, void* data_in)
{
	NODE2* node_new;

	node_new = (NODE2*)malloc (sizeof(NODE2));
	if (!node_new) return false;

	node_new->data = data_in;		//메모리 할당된 포인터
	node_new->next = stack->top;	//link
	node_new->prev = NULL;

	if (stack->count == 0) stack->bottom = node_new;
	else stack->top->prev = node_new;

	stack->top = node_new;	
	(stack->count)++;
	return true;
}

//스택의 bottom에 push한다.
bool stack_push_bottom (STACK* stack, void* data_in)
{
	NODE2* node_new;

	node_new = (NODE2*)malloc (sizeof(NODE2));
	if (!node_new) return false;

	node_new->data = data_in;		//메모리 할당된 포인터
	node_new->prev = stack->bottom; //link
	node_new->next = NULL;

	if (stack->count == 0) stack->top = node_new;
	else stack->bottom->next = node_new;

	stack->bottom = node_new;	
	(stack->count)++;
	return true;
}

//스택의 top에 push하고 스택 높이(height)가 유지되도록 bottom에서 제거.
bool stack_push_limit (STACK* stack, void* data_in, int height)
{
	NODE2* node_new;

	node_new = (NODE2*)malloc (sizeof(NODE2));
	if (!node_new) return false;

	node_new->data = data_in;
	node_new->next = stack->top; //link
	node_new->prev = NULL;

	if (stack->count == 0) stack->bottom = node_new;
	else {
		stack->top->prev = node_new;
		//스택높이 제한수치가 있으면, 스택 bottom에서 제거하여 높이를 유지한다.
		if (height) stack_remove_bottom (stack, height);
	}
	stack->top = node_new;	
	(stack->count)++;
	return true;
}

//스택이 height 높이 만큼 되도록 bottom에서 제거한다.(데이터 메모리 해제)
void stack_remove_bottom (STACK* stack, int height)
{
	if (stack->count >= height) {
		NODE2* node_temp;
		free (stack->bottom->data);
		node_temp = stack->bottom;
		stack->bottom = stack->bottom->prev;
		free (node_temp);
		(stack->count)--;
		
		stack_remove_bottom (stack, height);
	}
}

//스택 상단에서 데이터 가져옴 (상단 노드 제거)
void* stack_pop_top (STACK* stack, int flag)
{
	void* data_out;
	NODE2* node_temp;

	if (stack->top) {
		node_temp = stack->top;
		
		if (flag) {	//데이터 메모리 해제
			free (stack->top->data);
			data_out = NULL;
		} else data_out = stack->top->data;

		stack->top = stack->top->next; //link
		free (node_temp);
		(stack->count)--;
	} else return NULL;

	return data_out;
}

//스택 하단에서 데이터 가져옴 (하단 노드 제거)
void* stack_pop_bottom (STACK* stack, int flag)
{
	void* data_out;
	NODE2* node_temp;

	if (stack->bottom) {
		node_temp = stack->bottom;

		if (flag) {	//데이터 메모리 해제
			free (stack->bottom->data);
			data_out = NULL;
		} else data_out = stack->bottom->data;

		stack->bottom = stack->bottom->prev; //link
		free (node_temp);
		(stack->count)--;
	} else return NULL;

	return data_out;
}

//스택 상단에서 데이터 가져오고 top 포인터 이동
void* stack_top (STACK* stack, int flag)
{
	void* data_out;

	if (stack->top) {
		data_out = stack->top->data;
		if (flag) stack->top = stack->top->next; //link
	} else return NULL;

	return data_out;
}

//스택 하단에서 데이터 가져오고 bottom 포인터 이동
void* stack_bottom (STACK* stack, int flag)
{
	void* data_out;

	if (stack->bottom) {
		data_out = stack->bottom->data;
		if (flag) stack->bottom = stack->bottom->prev; //link
	} else return NULL;

	return data_out;
}

bool stack_is_empty (STACK* stack)
{
	return (stack->count == 0);
}

bool stack_is_full (STACK* stack)
{
	NODE2* node_temp;

	if ((node_temp = (NODE2*)malloc (sizeof(*(stack->top))) ) )	{
		free(node_temp);
		return false;
	}
	return true;
}

int stack_count (STACK* stack)
{
	return stack->count;
}
