//
//  Source: bpt2.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-05-12 B+Tree 코딩을 시작하다.
//		2010-12-08 헤더파일에 상세한 주석문을 추가하고 재정리하다.
//		2010-12-17 문자열키와 숫자키를 각각 작업할 수 있도록 NODE5를 수정하고 BTREE 구조체를 새로 추가하다.
//		yyyy-mm-dd ...
//

#include "dtype.h"

// OUTPUT -----------------------------------------------------------------------------------------

//new_node를 큐(qnode)에 추가 (bpt_print 함수에서 사용)
NODE5* _bpt_enqueue (NODE5* qnode, NODE5* new_node);

//node를 큐(qnode)에서 가져옴 (bpt_print 함수에서 사용)
NODE5* _bpt_dequeue (NODE5** qnode);

//B+ 트리 출력, 노드를 구분하기 위해 '|' 기호 사용, 키에 대한 포인터는 16진수로 출력
void bpt_print (BTREE* btree);

//리프 노드만 출력
void bpt_print_leaves (BTREE* btree);

//양방향 키 생성
//숫자키 --> 문자키
NODE5* bpt_init_key (BTREE*, BTREE*);
//번역키1 --> 번역키2 삽입
NODE5* bpt_init_trans_key (BTREE* t1, BTREE* hb[], int sh);

//트리 높이: 루트에서 리프까지 길이
int _bpt_height (NODE5* root);

//자식노드에서 루트노드까지 길이
int _bpt_path_to_root (NODE5* root, NODE5* child);

//루트에서 pointers을 따라가며 리프노드를 찾는다.
NODE5* bpt_find_leaf_debug (BTREE* btree, void* key, bool debug);
NODE5* bpt_find_leaf_unique (BTREE* btree, void* key, int (*compare)(void* p1, void* p2));
NODE5* bpt_find_leaf (BTREE* btree, void* key, int (*compare)(void* p1, void* p2));

//리프 노드를 찾아서 key 검색, leaf 넘겨 받음
int bpt_find_leaf_key (BTREE* btree, void* key, NODE5** leaf);
int bpt_find_leaf_key_next (BTREE* btree, void* key, NODE5** leaf, int count);
int bpt_find_leaf_key_trans (BTREE* btree, void* key, void* ptr, NODE5** leaf);
int bpt_find_leaf_key_like (BTREE* btree, void* key, NODE5** leaf);
int bpt_find_leaf_key_similar (BTREE* btree, void* key, NODE5** leaf);

//리프를 찾아서 key를 검색하여 해당 record를 반환한다.
void* bpt_search (BTREE* btree, void* key);
void* bpt_search_str_unique_like (BTREE* btree, void* key);

//Order(노드안의 배열 요소수)의 중간 위치 계산
//((Order + 1) / 2) - 1
int _bpt_half_order (int length);


// INSERTION --------------------------------------------------------------------------------------

//B+ 트리에 key와 value을 삽입한다.
/*
 1.bpt_insert ()	
 2.	_bpt_make_root () //루트 노드 최초 생성
 3.	bpt_find_leaf ()
 4.	_bpt_make_record ()

	//리프 노드에 충분한 엔트리가 있으면
 5.	_bpt_insert_into_leaf () -->return
	
	//리프 노드에 충분한 엔트리가 없으면(꽉차 있으면)
 6.	_bpt_insert_into_leaf_after_splitting ()  //리프분할

 7.		_bpt_insert_into_parent ()  //분할된 노드를 부모에 연결
 8.			_bpt_insert_into_new_root () 			
 9.			_bpt_get_left_index ()  //부모노드에서 왼쪽 자식의 인덱스

			//부모 노드에 충분한 엔트리가 있으면
10.			_bpt_insert_into_parent_node () --> return

			//부모 노드에 충분한 엔트리가 없으면(꽉차 있으면)
11.			_bpt_insert_into_parent_after_splitting ()  //부모노드 다시 분할
12.				_bpt_insert_into_parent ()  //7번으로 이동(반복)
*/

//btree 구조체 생성
BTREE* bpt_create (int order, int (*compare)(void* p1, void* p2)
				   , void (*outkey)(void* p1), void (*outdata)(void* p1), bool samek );

//리프노드에 키와 데이터 입력
NODE5* bpt_insert (BTREE* btree, NODE5* leaf, void* key, void* data, int flag);

//키 순서대로 입력되는 경우 입력 속도 개선(leaf 연결을 그대로 사용)
NODE5* bpt_insert_asc (BTREE* btree, NODE5** leaf, void* key, void* data);

//루트 노드 생성
NODE5* _bpt_make_root (BTREE* btree, void* key, void* data);

//리프 노드 생성
NODE5* _bpt_make_leaf (BTREE* btree);

//노드 생성(메모리 할당)
NODE5* _bpt_make_node (BTREE* btree);

//RECORD 하나를 생성한다.(메모리 할당)
//RECORD* _bpt_make_record (void* record);

//리프노드에 삽입한다.
NODE5* _bpt_insert_into_leaf (NODE5* leaf, BTREE* btree, void* key, void* data);

//키가 큰 순서대로 입력되는 경우, 배열 끝에 그냥 삽입(속도 개선)
NODE5* _bpt_insert_into_leaf_asc (NODE5* leaf, BTREE* btree, void* key, void* data);

//리프를 분리한후 삽입한다.
NODE5* _bpt_insert_into_leaf_after_splitting (BTREE* btree, NODE5* leaf, void* key, void* data);

//새로운 키(오른쪽 노드의 첫번째 키)를 부모노드에 삽입한다.
NODE5* _bpt_insert_into_parent (BTREE* btree, NODE5* left, void* key, NODE5* right);

//새로운 부모 노드 생성 (트리높이 증가)
NODE5* _bpt_insert_into_new_root (BTREE* btree, NODE5* left, void* key, NODE5* right);

//부모 노드에서 왼쪽 노드의 위치를 찾음
int _bpt_get_left_index (NODE5* parent, NODE5* left);

//부모노드에 삽입
NODE5* _bpt_insert_into_parent_node (NODE5* root, NODE5* node, int left_index, void* key, NODE5* right);

//부모노드 분할
NODE5* _bpt_insert_into_parent_after_splitting (BTREE* btree, NODE5* left, int left_index, void* key, NODE5* right);


// DELETION ---------------------------------------------------------------------------------------

//key에 해당하는 노드 삭제
/*
1.bpt_delete ()
2.	bpt_delete_entry ()
3.		_bpt_remove_entry_from_node ()  //엔트리 삭제
4.		_bpt_adjust_root ()  //루트 노드 재조정
5.		_bpt_get_neighbor_index ()  //이웃(왼쪽) 노드 인덱스

		//이웃(왼쪽) 노드와 충분한 엔트리가 있으면 노드 병합(노드 다시 삭제 발생함)
6.		_bpt_coalesce_nodes ()
			bpt_delete_entry () //2번으로 이동(반복)

		//이웃(왼쪽) 노드에 충분한 엔트리가 없으면(꽉차 있으면) 노드 재분배
7.		_bpt_redistribute_nodes ()
*/
//단독 B+트리 노드 삭제
NODE5* bpt_delete (BTREE* btree, void* key, bool *deleted);

//노드안의 엔트리 삭제(반복 호출됨)
NODE5* bpt_delete_entry (BTREE* btree, NODE5* node, void* key, void* pointer, int flag);

//노드의 엔터리(배열)에서 키-포인터 삭제
NODE5* _bpt_remove_entry_from_node (BTREE* btree, NODE5* node, void* key, void* pointer, int flag);

//루트노드가 비어 있다면 재조정
NODE5* _bpt_adjust_root (NODE5* root);

//이웃(왼쪽)노드 인덱스
int _bpt_get_neighbor_index (NODE5* node);

//이웃(왼쪽) 노드에 충분한 엔트리가 있으면 노드 병합(함침, 남는 노드는 삭제)
NODE5* _bpt_coalesce_nodes (BTREE* btree, NODE5* node, NODE5* neighbor, int neighbor_index, void* kp);

//이웃(왼쪽) 노드에 충분한 엔트리가 없으면(꽉차 있으면) 노드 재분배
NODE5* _bpt_redistribute_nodes (NODE5* root, NODE5* node, NODE5* neighbor, int neighbor_index, int kp_index, void* kp);


// DESTROY ----------------------------------------------------------------------------------------

//리프노드에 할당된 메모리와 노드 포인터들을 동시에 해제(재귀호출)
unsigned int bpt_drop_leaves_nodes (BTREE* btree, NODE5* node);

//리프노드에 할당된 메모리 해제
unsigned int bpt_drop_leaves (BTREE* btree, NODE5* node);
//노드 포인터 해제(재귀호출)
unsigned int bpt_drop_nodes (NODE5* node);

//모든 노드 삭제
void bpt_drop (BTREE**, BTREE**);

//모든 노드와 B+트리까지 메모리 해제
void bpt_drop_all (BTREE** ws, BTREE** wi);

