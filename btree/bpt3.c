//
//  Source: bpt3.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(c): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-05-12 B+Tree 코딩을 시작하다.
//		2010-12-06 삽입 및 삭제시 bpt_find_leaf() 함수가 2번 호출되는 것을 1번으로 줄임.(트리탐색 중복제거)
//		2010-12-07 노드 삭제시 재분배 함수 _bpt_redistribute_nodes()에 있는 버그를 수정하다.
//		2010-12-11 노드 삭제시 병합 함수 _bpt_coalesce_nodes()에서 불필요한 코드(split)를 제거하다.
//		2010-12-17 문자열키와 숫자키를 각각 작업할 수 있도록 B+Tree 함수들을 수정하다.
//		2010-12-18 노드 삭제시 메모리 해제되지 않는 문제 수정하다. _bpt_remove_entry_from_node() 수정.
//		2011-01-17 중복키 허용을 위한 B+트리 입출력 함수들 수정하다.
//		yyyy-mm-dd ...
//

#include <stdio.h>
#include <stdlib.h>

#include "dtype.h"
#include "ustr.h"
#include "tw1.h"
#include "bpt3.h"

//alloca() 함수 헤더
#ifdef __LINUX
	#include <alloca.h>	//Linux
#else
	#include <malloc.h>		//Win
#endif

// OUTPUT ---------------------------------------------------------------------------------------

//new_node를 큐(qnode)에 추가 (bpt_print 함수에서 사용)
NODE5* _bpt_enqueue (NODE5* qnode, NODE5* new_node)
{
	NODE5* node;

	if (qnode == NULL) {
		qnode = new_node;
		qnode->next = NULL;

	} else {
		node = qnode;
		while(node->next != NULL) node = node->next;

		node->next = new_node;
		new_node->next = NULL;
	}
	return qnode;
}

//node를 큐(qnode)에서 가져옴 (bpt_print 함수에서 사용)
NODE5* _bpt_dequeue (NODE5** qnode)
{
	NODE5* node = *qnode;

	*qnode = (*qnode)->next;
	node->next = NULL;
	return node;
}

//B+ 트리 출력, 노드를 구분하기 위해 '|' 기호 사용, 키에 대한 포인터는 16진수로 출력
void bpt_print (BTREE* btree)
{
	NODE5* node = NULL;
	NODE5* qnode = NULL;  //출력용 큐 노드

	register int i = 0, j = 0;
	int rank = 0;
	int new_rank = 0;

	if (!btree->root) {
		printf("Empty.\n");
		return;
	}

	qnode = _bpt_enqueue (qnode, btree->root);
	while (qnode)
	{
		node = _bpt_dequeue (&qnode);
		if (node->parent && node == node->parent->pointers[0]) {
			new_rank = _bpt_path_to_root (btree->root, node);
			if (new_rank != rank) {
				rank = new_rank;
				printf("\n");
			}
		}
		for (i = 0; i < node->num_keys; i++) {
			btree->outkey (node->keys[i]);
			printf (" ");
			if (!node->is_leaf)
				for (j = 0; j <= node->num_keys; j++)
					qnode = _bpt_enqueue (qnode, node->pointers[j]);
		} //for
		printf("| ");
	} //while
	printf ("\n");
	printf ("Height=%d.\n", _bpt_height (btree->root));
	printf ("\n");
}

//리프 노드만 출력
void bpt_print_leaves (BTREE* btree)
{
	register int i, height;
	unsigned int keys_cnt = 0;
	NODE5*		 node = btree->root;

	if (!node) {
		printf("Empty.\n");
		return;
	}

	//첫번째 리프 노드 찾음
	height = 1;
	while (!node->is_leaf) {
		node = (NODE5*)node->pointers[0];
		height++;  //트리 높이
	}

	while (true) {
		keys_cnt += node->num_keys;  //리프에 있는 키들의 수 덧셈
		for (i = 0; i < node->num_keys; i++) {
			btree->outkey (node->keys[i]);
			printf (":");
			btree->outdata (node->pointers[i]);
			printf (" ");
		}

		if (!(keys_cnt % 50))
			if (tw1_qn_answer ("\n** Would you like to see more? [Y/n]", FLAG_YES) == FLAG_NO) break;

		if (node->pointers[btree->order - 1]) {
			printf(" | ");
			node = (NODE5*)node->pointers[btree->order - 1];	//다음 리프 노드
		} else break;
	} //while

	printf ("\n");
	printf ("Height(%d), Kno(%d), Kcnt(%d==%d)\n", height, btree->kno, keys_cnt, btree->kcnt);
}

//숫자키에서 문자키 B+트리 생성 (wi --> ws)
//엔트리가 100% 채워지지 않음
NODE5* bpt_init_key (BTREE* wi, BTREE* ws)
{
	register int i;
	NODE5*  leaf;
	NODE5*	node = wi->root;

	if (!node) {
		printf("Empty.\n");
		return NULL;
	}
	while (!node->is_leaf)
		node = (NODE5*)node->pointers[0];  //숫자키 리프 노드까지 이동
	while (true) {
		//리프노드를 처음부터 끝까지 삽입
		for (i = 0; i < node->num_keys; i++) {
			leaf = bpt_find_leaf (ws, node->pointers[i], ws->compare);
			ws->root = bpt_insert (ws, leaf, node->pointers[i], node->keys[i], FLAG_INSERT);	//문자키 삽입
		}
		if (node->pointers[wi->order - 1])
			node = (NODE5*)node->pointers[wi->order - 1];	//숫자키의 다음 리프 노드
		else break;
	} //while

	return ws->root;
}

//번역키1 --> 번역키2 삽입
NODE5* bpt_init_trans_key (BTREE* t1, BTREE* hb[], int sh)
{
	register int i;
	NODE5*  leaf;
	NODE5*	node = t1->root;
	unsigned int h;

	if (!node) {
		printf("Empty.\n");
		return NULL;
	}
	while (!node->is_leaf)
		node = (NODE5*)node->pointers[0];  //리프 노드까지 이동
	while (true) {
		//리프노드를 처음부터 끝까지 삽입
		for (i = 0; i < node->num_keys; i++) {
			h = (sh == 0) ? sh : hash_value (node->pointers[i]); //해시값
			leaf = bpt_find_leaf (hb[h], node->pointers[i], hb[h]->compare);
			hb[h]->root = bpt_insert (hb[h], leaf, node->pointers[i], node->keys[i], FLAG_INSERT);
		}
		if (node->pointers[t1->order - 1])
			node = (NODE5*)node->pointers[t1->order - 1];	//다음 리프 노드
		else break;
	} //while

	return t1->root;
}

//트리 높이: 루트에서 리프까지 길이
int _bpt_height (NODE5* root)
{
	register int h = 0;

	NODE5*	node = root;

	while (!node->is_leaf) {
		node = node->pointers[0];
		h++;
	}
	return h+1;
}

//자식노드에서 루트노드까지 길이
int _bpt_path_to_root (NODE5* root, NODE5* child)
{
	register int length = 0;
	NODE5*	node = child;

	while (node != root) {
		node = node->parent;
		length++;
	}
	return length;
}

///디버깅용
NODE5* bpt_find_leaf_debug (BTREE* btree, void* key, bool debug)
{
	int		i=0, height=0;
	NODE5*	node = btree->root;

	if (node == NULL) {
		if (debug) printf("Empty tree.\n");
		return node;
	}
	//리프노드까지(트리높이 만큼)
	while (!node->is_leaf) {
		height++;
		if (debug) {
			printf ("%d: (", height);
			for (i = 0; i < node->num_keys; i++) {	//노드의 키들을 출력(debug)
				printf (",");
				btree->outkey (node->keys[i]);
			}
			printf (",) ");
		}
		i = 0;
		while (i < node->num_keys) {  //노드안의 키배열을 순차적으로 탐색
			if (btree->compare (key, node->keys[i]) < 0) break;  //(key < node->keys[i])
			else i++;
		}
		if (debug) printf("-> %d th\n", i);

		node = (NODE5 *)node->pointers[i];
	} //while

	if (debug) {
		printf ("%d: [", ++height);  //Leaf
		for (i = 0; i < node->num_keys; i++) {
			printf (",");
			btree->outkey (node->keys[i]);
			if (btree->compare (key, node->keys[i]) == 0) printf ("(*)");  //찾음(node->keys[i] == key)
			//printf (":");
			//btree->outdata (node->pointers[i]);
		}
		printf (" *]\n\n");
	}
	return node;
}

//유일키(중복없는) 검색
NODE5* bpt_find_leaf_unique (BTREE* btree, void* key, int (*compare)(void* p1, void* p2))
{
	register int i;
	NODE5*	node = btree->root;

	if (!node) return NULL;

	//리프노드까지(트리높이 만큼)
	while (!node->is_leaf) {
		i = 0;
		while (i < node->num_keys) {  //노드안의 키배열을 순차적으로 탐색
			if (compare (key, node->keys[i]) < 0) break;
			else i++;
		}
		node = (NODE5 *)node->pointers[i];
	} //while
	return node;
}

//루트에서 pointers을 따라가며 리프노드로 이동
//중복키 허용 코드 추가(2011-01-18)
NODE5* bpt_find_leaf (BTREE* btree, void* key, int (*compare)(void* p1, void* p2))
{
	register int i, k = 1;
	NODE5	*node;
	bool	in_key = false;	//internal key 여부

	node = btree->root;
	if (!node) return NULL;

	//리프노드까지 이동
	while (!node->is_leaf) {
		i = 0;
		while (i < node->num_keys) {  //노드안의 키배열을 순차적으로 탐색
			k = btree->compare (key, node->keys[i]);
			if (btree->samek && k <= 0) break;	//k==0,내부키
			else if (k < 0) break;
			else i++;
		}
		if (k == 0) in_key = true;	//내부키
		node = (NODE5 *)node->pointers[i];
	} //while

	//내부키가 발생했고 중복키 허용이라면, 리프에서 키를 순차 검색
	if (in_key && btree->samek) {
		do {
			for (i = 0; i < node->num_keys; i++)
				if (compare (key, node->keys[i]) <= 0) break;
		} while ((i == node->num_keys) && (node = node->pointers [btree->order - 1]));
	}
	return node;  //리프 노드
}

//리프 노드를 찾아서 동일한 키가 있는지 검색
int bpt_find_leaf_key (BTREE* btree, void* key, NODE5** leaf)
{
	register int i;

	*leaf = bpt_find_leaf (btree, key, btree->compare);
	if (! *leaf) return -2;  //리프노드 없음

	for (i = 0; i < (*leaf)->num_keys; i++)
		if (btree->compare (key, (*leaf)->keys[i]) == 0) break;

	return (i < (*leaf)->num_keys) ? i : -1;
}

//동일한 키가 주어진 회수 만큼 있는지 검색
int bpt_find_leaf_key_next (BTREE* btree, void* key, NODE5** leaf, int count)
{
	register int i;
	int idx=0, cnt=0;

	*leaf = bpt_find_leaf (btree, key, btree->compare);
	if (! *leaf) return -2;  //리프노드 없음

	for (i = 0; i < (*leaf)->num_keys; i++) {
		if (btree->compare (key, (*leaf)->keys[i]) == 0) {
			idx = i;
			cnt++;
		}
		if (cnt == count) break;
	}
	while ((cnt < count) && (idx == i-1) ) {
		*leaf = (*leaf)->pointers [btree->order - 1];  //다음 리프
		if (!*leaf) return -1;  //키 없음
		for (i = 0; i < (*leaf)->num_keys; i++) {
			if (btree->compare (key, (*leaf)->keys[i]) == 0) {
				idx = i;
				cnt++;
			}
			if (cnt == count) break;
		}
	} //while

	return (i < (*leaf)->num_keys) ? i : -1;
}

//번역키 리프 노드를 찾음 (키중복이 있으므로 데이터 포인터까지 비교)
int bpt_find_leaf_key_trans (BTREE* btree, void* key, void* ptr, NODE5** leaf)
{
	register int i;

	*leaf = bpt_find_leaf (btree, key, btree->compare);
	if (! *leaf) return -2;  //리프노드 없음

	for (i = 0; i < (*leaf)->num_keys; i++)
		//if ( (btree->compare ((*leaf)->keys[i], key) == 0) && ((*leaf)->pointers[i] == ptr) ) break;
		if ( ((*leaf)->keys[i] == key) && ((*leaf)->pointers[i] == ptr) ) break;

	return (i < (*leaf)->num_keys) ? i : -1;
}

///리프 노드를 찾아서 key까지만 비교(key까지만 일치하는지 여부, 중복키 허용)
int bpt_find_leaf_key_like (BTREE* btree, void* key, NODE5** leaf)
{
	register int i;
	char ucmax[ASIZE], key2[SSIZE];

	*leaf = bpt_find_leaf (btree, key, str_cmp_int_like);  //중복키 허용
	if (! *leaf) return -2;  //리프노드 없음
	for (i = 0; i < (*leaf)->num_keys; i++)
		if (str_cmp_int_like (key, (*leaf)->keys[i]) == 0) break;

	if (i < (*leaf)->num_keys) return i;  //키 있음(키인덱스)

	//key의 끝에 최대값을 하나 추가한다.
	//리프를 다시 찾는다.
	uint_to_str (UIFAIL, ucmax);	//unsigned int 최대값
	str_copy (key2, key);
	str_cat (key2, ucmax);
	str_cat (key2, "_");
	//printf ("** Finding like key2:%s\n", key2);

	*leaf = bpt_find_leaf (btree, key2, str_cmp_int_like);  //중복키 허용
	//*leaf = bpt_find_leaf_debug  (btree, key2, true);
	if (! *leaf) return -2;  //리프노드 없음

	for (i = 0; i < (*leaf)->num_keys; i++)
		if (str_cmp_int_like (key, (*leaf)->keys[i]) == 0) break;

	return (i < (*leaf)->num_keys) ? i : -1;
}

///key가 중간에 일치하는 것 검색
//리프 노드를 처음부터 순서대로 검색(full scan)
int bpt_find_leaf_key_similar (BTREE* btree, void* key, NODE5** leaf)
{
	NODE5*	node = btree->root;
	register int i;
	char	prompt[] = {'+', '-', '*'};
	unsigned int sno = 0;

	//리프토드로 이동
	while (!node->is_leaf) node = (NODE5 *)node->pointers[0];
	*leaf = node;
	if (! *leaf) return -2;  //리프노드 없음

	//리프노드 전체검색(full scan)
	do {
		for (i = 0; i < (*leaf)->num_keys; i++) {
			//btree->outkey ((*leaf)->keys[i]);
			sno++;
			printf ("%c**(%c)%u: %s", CR, prompt[sno%3], sno, "Finding similar words...");
			if (str_cmp_int_similar (key, (*leaf)->keys[i]) == 0 ) break;
		}
	} while ((i == (*leaf)->num_keys) && (*leaf = (*leaf)->pointers [btree->order -1]) );
	//printf ("\n\n");

	if (! *leaf) return -1;
	return (i < (*leaf)->num_keys) ? i : -1;
}

//리프를 찾아서 key를 검색하여 해당 record를 반환한다.
void* bpt_search (BTREE* btree, void* key)
{
	register int i = 0;
	NODE5* leaf;

	leaf = bpt_find_leaf (btree, key, btree->compare);
	if (!leaf) return NULL;

	for (i = 0; i < leaf->num_keys; i++)
		if (btree->compare (key, leaf->keys[i]) == 0) break;

	return (i == leaf->num_keys) ? NULL : leaf->pointers[i];
}

///문자열 앞이 일치하는 단어 반환 (탭키 검색)
void* bpt_search_str_unique_like (BTREE* btree, void* key)
{
	register int i = 0;
	NODE5	*leaf, *leaf2;
	int		k, k2, cnt = 0;

	//leaf = bpt_find_leaf_debug (btree, key, true);
	leaf = bpt_find_leaf_unique (btree, key, btree->compare);
	//리프에서 str_cmp_like로 검색
	do {
		for (i = 0; i < leaf->num_keys; i++)
			if (str_cmp_like (key, leaf->keys[i]) == 0) break;
	} while ((i == leaf->num_keys) && (leaf = leaf->pointers [btree->order -1]) );

	if (!leaf) return NULL;

	k = i;
	k2 = i;
	leaf2 = leaf;
	do {
		for (i = k2; i < leaf2->num_keys; i++) {
			if (str_cmp_like (key, leaf2->keys[i]) == 0) {
				btree->outkey (leaf2->keys[i]);	//일치 하는것이 있으면 계속 출력
				printf ("\t");
				cnt++;
			} else break;
		}
		k2 = 0;
	} while ((i == leaf2->num_keys) && (leaf2 = leaf2->pointers [btree->order -1]) );

	return (cnt == 1) ? leaf->keys[k] : NULL;	//key 반환(1개만 있을때)
}

//Order(노드안의 배열 요소수)의 중간 위치 계산
//((Order + 1) / 2) - 1
int _bpt_half_order (int length)
{
	return (length % 2 == 0) ? length / 2 : length / 2 + 1;
}


// INSERTION --------------------------------------------------------------------------------------
/*
1. 리프에 삽입
2. 리프 분할
3. 부모에 삽입
4. 부모분할(3번에서 반복)
*/

/*
 1.bpt_insert ()
 2.	_bpt_make_root () //루트 노드 최초 생성
 3.	bpt_find_leaf ()
 4.	_bpt_make_record ()

	//리프 노드에 충분한 엔트리가 있으면
 5.	_bpt_insert_into_leaf () -->return

	//리프 노드에 충분한 엔트리가 없으면(꽉차 있으면)
 6.	_bpt_insert_into_leaf_after_splitting ()  //리프분할(새로운 노드 right 할당)

_7.		_bpt_insert_into_parent ()  //분할된 노드를 부모에 연결
 8.			_bpt_insert_into_new_root ()
 9.			_bpt_get_left_index ()  //부모노드에서 왼쪽 자식의 인덱스

			//부모 노드에 충분한 엔트리가 있으면
10.			_bpt_insert_into_parent_node () --> return

			//부모 노드에 충분한 엔트리가 없으면(꽉차 있으면)
11.			_bpt_insert_into_parent_after_splitting ()  //부모노드 다시 분할(새로운 노드 right 할당)
12.				_bpt_insert_into_parent ()  //7번으로 이동(반복)
*/

//B+ 트리 최초 생성
BTREE* bpt_create (int order, int (*compare)(void* p1, void* p2)
				   , void (*outkey)(void* p1), void (*outdata)(void* p1), bool samek )
{
	BTREE*  btree;

	btree = (BTREE*) malloc (sizeof(BTREE));
	if (btree) {
		btree->order = order;		//엔터리 배열크기
		btree->kno  = 0;			//키고유번호(증가)
		btree->kcnt = 0;			//키개수(증감)
		btree->root = NULL;
		btree->compare = compare;	//비교 함수포인터
		btree->outkey = outkey;		//키출력 함수포인터
		btree->outdata = outdata;	//데이터출력 함수포인터
		btree->samek = samek;		//중복키 허용여부
	}
	return btree;
}

//리프노드에 키와 데이터 입력
NODE5* bpt_insert (BTREE* btree, NODE5* leaf, void* key, void* data, int flag)
{
	if (flag==FLAG_INSERT)	btree->kno++;	//키 고유번호 증가
	btree->kcnt++;  //키 개수 증가

	//B+ 트리가 아직 존재하지 않는다면 처음으로 생성
	if (btree->root == NULL) {
		//레코드를 메모리에 할당한다.
		//pointer = _bpt_make_record (btree->record);
		btree->root = _bpt_make_root (btree, key, data);
		return btree->root;
	}

	//레코드를 메모리 할당한다.
	//pointer = _bpt_make_record (key, data);

	//리프노드 overflow 아님: 리프 노드 중간에 삽입
	if (leaf->num_keys < btree->order - 1) {
		leaf = _bpt_insert_into_leaf (leaf, btree, key, data);
		return btree->root;
	}
	//리프노드 overflow: 리프를 분리한후 삽입한다.
	return _bpt_insert_into_leaf_after_splitting (btree, leaf, key, data);
}

//키 순서대로 입력되는 경우 입력 속도 개선(leaf 연결을 그대로 사용)
NODE5* bpt_insert_asc (BTREE* btree, NODE5** leaf, void* key, void* data)
{
	btree->kno++;	//키 고유번호 증가
	btree->kcnt++;  //키 개수 증가

	//B+ 트리가 아직 존재하지 않는다면 처음으로 생성
	if (btree->root == NULL) {
		btree->root = _bpt_make_root (btree, key, data);
		*leaf = btree->root;
		return btree->root;
	}

	//리프노드 overflow 아님: 리프 배열 끝에 삽입
	if ((*leaf)->num_keys < btree->order - 1) {
		*leaf = _bpt_insert_into_leaf_asc (*leaf, btree, key, data);
		return btree->root;
	}
	//리프노드 overflow: 리프를 분리한후 삽입한다.
	btree->root = _bpt_insert_into_leaf_after_splitting (btree, *leaf, key, data);

	//리프가 분리된 경우, 다음 리프(오른쪽으로 연결된 리프) 반환
	*leaf = (*leaf)->pointers[btree->order - 1];

	return btree->root;
}

NODE5* _bpt_make_root (BTREE* btree, void* key, void* data)
{
	NODE5* root = _bpt_make_leaf (btree);	///리프노드 생성(메모리할당)

	root->keys[0] = key;
	root->pointers[0] = data;
	root->pointers[btree->order - 1] = NULL;
	root->parent = NULL;
	root->num_keys++;

	return root;
}

///리프 노드 생성
NODE5* _bpt_make_leaf (BTREE* btree)
{
	NODE5* leaf = _bpt_make_node (btree);
	leaf->is_leaf = true;
	return leaf;
}

///노드 생성(메모리 할당)
NODE5* _bpt_make_node (BTREE* btree)
{
	NODE5* new_node;

	new_node = malloc(sizeof(NODE5));
	if (new_node == NULL) {
		printf ("## Node creation error in _bpt_make_node().\n");
		exit(EXIT_FAILURE);
	}
	new_node->keys = malloc ( (btree->order - 1) * sizeof(void*) );
	if (new_node->keys == NULL) {
		printf ("## New NODE5 keys array in _bpt_make_node().\n");
		exit(EXIT_FAILURE);
	}
	new_node->pointers = malloc ( btree->order * sizeof(void*) );
	if (new_node->pointers == NULL) {
		printf ("## New NODE5 pointers array in _bpt_make_node().\n");
		exit(EXIT_FAILURE);
	}
	new_node->is_leaf = false;
	new_node->num_keys = 0;
	new_node->parent = NULL;
	new_node->next = NULL;

	return new_node;
}

//RECORD 하나를 생성한다.(메모리 할당)
/*
RECORD* _bpt_make_record (void* record)
{
	RECORD* new_record = (RECORD *)malloc (sizeof(RECORD));

	if (new_record == NULL) {
		printf ("## Record creation error in _bpt_make_record().\n");
		exit (EXIT_FAILURE);
	} else {
		new_record->key = ((RECORD *)record)->key;
		new_record->word = ((RECORD *)record)->word;
	}
	return new_record;
}
*/

//리프노드에 삽입한다.
NODE5* _bpt_insert_into_leaf (NODE5* leaf, BTREE* btree, void* key, void* data)
{
	register int i, idx;

	idx = 0;
	//삽입할 위치를 찾는다.(=는 중복키)
	while (idx < leaf->num_keys && btree->compare (leaf->keys[idx], key) <= 0)
		idx++;

	for (i = leaf->num_keys; i > idx; i--) {
		leaf->keys[i] = leaf->keys[i - 1];	//오른쪽으로 이동
		leaf->pointers[i] = leaf->pointers[i - 1];
	}
	//삽입위치(idx)에 삽입
	leaf->keys[idx] = key;
	leaf->pointers[idx] = data;
	leaf->num_keys++;

	return leaf;
}

//키가 큰 순서대로 입력되는 경우, 배열 끝에 그냥 삽입
NODE5* _bpt_insert_into_leaf_asc (NODE5* leaf, BTREE* btree, void* key, void* data)
{
	//배열끝에 삽입
	leaf->keys[leaf->num_keys] = key;
	leaf->pointers[leaf->num_keys] = data;
	leaf->num_keys++;

	return leaf;
}

//리프를 분리한후 삽입한다.
NODE5* _bpt_insert_into_leaf_after_splitting (BTREE* btree, NODE5* leaf, void* key, void* data)
{
	NODE5*	new_leaf;
	void**	temp_keys;
	void**	temp_pointers;
	void*	new_key;
	int		idx, split, i, j;

	new_leaf = _bpt_make_leaf (btree);	//새로운 리프노드 메모리할당

	//임시 메모리 할당
	//temp_keys = malloc (btree->order * sizeof(void*) );
	temp_keys = alloca (btree->order * sizeof(void*) );
	if (temp_keys == NULL) {
		printf ("## Temporary keys allocation error in _bpt_insert_into_leaf_after_splitting().\n");
		exit(EXIT_FAILURE);
	}
	//temp_pointers = malloc (btree->order * sizeof(void*) );
	temp_pointers = alloca (btree->order * sizeof(void*) );
	if (temp_pointers == NULL) {
		printf ("## Temporary pointers allocation error in _bpt_insert_into_leaf_after_splitting().\n");
		exit(EXIT_FAILURE);
	}

	idx = 0;
	//삽입할 위치를 찾는다.(=는 중복키)
	while (idx < btree->order - 1 && btree->compare (leaf->keys[idx], key) <= 0)
		idx++;

	//leaf를 temp로 복사
	for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
		if (j == idx) j++; //key위치는 건너뜀
		temp_keys[j] = leaf->keys[i];
		temp_pointers[j] = leaf->pointers[i];
	}
	temp_keys[idx] = key;	//key삽입
	temp_pointers[idx] = data;

	leaf->num_keys = 0;
	//리프노드 분리 인덱스
	split = _bpt_half_order (btree->order - 1);

	//split 이하(Left)를 left로 이동
	for (i = 0; i < split; i++) {
		leaf->pointers[i] = temp_pointers[i];
		leaf->keys[i] = temp_keys[i];
		leaf->num_keys++;
	}
	//split 이상(Right)을 new_left로 이동
	for (i = split, j = 0; i < btree->order; i++, j++) {
		new_leaf->pointers[j] = temp_pointers[i];
		new_leaf->keys[j] = temp_keys[i];
		new_leaf->num_keys++;
	}

	//임시 메모리 삭제 (alloca로 스택에 할당된 메모리는 scope 벗어나면 자동 해제됨)
	//free(temp_pointers);
	//free(temp_keys);

	//제일 오른쪽 포인터(이웃 연결)
	new_leaf->pointers[btree->order - 1] = leaf->pointers[btree->order - 1];
	leaf->pointers[btree->order - 1] = new_leaf;

	//뒷부분 NULL 처리
	for (i = leaf->num_keys; i < btree->order - 1; i++)
		leaf->pointers[i] = NULL;
	for (i = new_leaf->num_keys; i < btree->order - 1; i++)
		new_leaf->pointers[i] = NULL;

	//부모노드 처리
	new_leaf->parent = leaf->parent;
	//새로운 노드(Right)의 첫번째 키
	new_key = new_leaf->keys[0];

	//new_key를 부모노드에 삽입한다.
	return _bpt_insert_into_parent (btree, leaf, new_key, new_leaf);	//(root, left, key, right)
}

//새로운 키(오른쪽 노드의 첫번째 키)를 부모노드에 삽입한다.
NODE5* _bpt_insert_into_parent (BTREE* btree, NODE5* left, void* key, NODE5* right)
{
	register int left_index;
	NODE5*	parent;

	parent = left->parent;
	if (parent == NULL) //새로운 부모 노드 생성 (트리높이 증가)
		return _bpt_insert_into_new_root (btree, left, key, right);

	//부모 노드에서 왼쪽 노드의 위치를 찾음
	left_index = _bpt_get_left_index (parent, left);

	if (parent->num_keys < btree->order - 1)
		//부모노드에 삽입
		return _bpt_insert_into_parent_node (btree->root, parent, left_index, key, right);

	//부모노드 분할(overflow)
	return _bpt_insert_into_parent_after_splitting (btree, parent, left_index, key, right);
}

//새로운 부모 노드 생성 (트리높이 증가)
NODE5* _bpt_insert_into_new_root (BTREE* btree, NODE5* left, void* key, NODE5* right)
{
	NODE5* root = _bpt_make_node (btree);

	root->keys[0] = key;
	root->pointers[0] = left;
	root->pointers[1] = right;
	root->num_keys++;
	root->parent = NULL;
	left->parent = root;
	right->parent = root;

	return root;
}

//부모 노드에서 왼쪽 노드의 위치를 찾음
int _bpt_get_left_index (NODE5* parent, NODE5* left)
{
	register int left_index = 0;

	while (left_index <= parent->num_keys && parent->pointers[left_index] != left)
		left_index++;

	return left_index;
}

//부모노드에 삽입
NODE5* _bpt_insert_into_parent_node (NODE5* root, NODE5* node, int left_index, void* key, NODE5* right)
{
	register int i;

	for (i = node->num_keys; i > left_index; i--) {
		node->pointers[i + 1] = node->pointers[i];	//오른쪽으로 이동
		node->keys[i] = node->keys[i - 1];
	}
	node->pointers[left_index + 1] = right;
	node->keys[left_index] = key;
	node->num_keys++;

	return root;
}

//부모노드 분할
NODE5* _bpt_insert_into_parent_after_splitting (BTREE* btree, NODE5* left, int left_index, void* key, NODE5* right)
{
	register int i, j, split;
	NODE5*	new_node, * child;
	void**	temp_keys;
	NODE5** temp_pointers;
	void*	kp;

	//임시 메모리 할당(split 전에 포인터는 Order보다 한개 많음)
	//temp_pointers = malloc ((btree->order + 1) * sizeof(NODE5*) );
	temp_pointers = alloca ((btree->order + 1) * sizeof(NODE5*) );
	if (temp_pointers == NULL) {
		printf ("## Temporary pointers allocation error in _bpt_insert_into_parent_after_splitting().\n");
		exit(EXIT_FAILURE);
	}
	//temp_keys = malloc (btree->order * sizeof(void*) );
	temp_keys = alloca (btree->order * sizeof(void*) );
	if (temp_keys == NULL) {
		printf ("## Temporary keys allocation error in _bpt_insert_into_parent_after_splitting().\n");
		exit(EXIT_FAILURE);
	}

	//포인터를 임시 메모리에 복사
	for (i = 0, j = 0; i < left->num_keys + 1; i++, j++) {
		if (j == left_index + 1) j++;
		temp_pointers[j] = left->pointers[i];
	}
	//키를 임시 메모리에 복사
	for (i = 0, j = 0; i < left->num_keys; i++, j++) {
		if (j == left_index) j++;
		temp_keys[j] = left->keys[i];
	}
	//삽입
	temp_pointers[left_index + 1] = right;
	temp_keys[left_index] = key;

	//새로운 노드 할당(right)
	new_node = _bpt_make_node (btree);

	//내부노드는 분리 인덱스가 하나 더 많음
	split = _bpt_half_order (btree->order);
	left->num_keys = 0;
	//반으로 나눔(left)
	for (i = 0; i < split - 1; i++) {
		left->pointers[i] = temp_pointers[i];
		left->keys[i] = temp_keys[i];
		left->num_keys++;
	}
	left->pointers[i] = temp_pointers[i];
	kp = temp_keys[split - 1];  //부모로 올라가는 키

	//반으로 나눔(right)
	for (++i, j = 0; i < btree->order; i++, j++) {
		new_node->pointers[j] = temp_pointers[i];
		new_node->keys[j] = temp_keys[i];
		new_node->num_keys++;
	}
	new_node->pointers[j] = temp_pointers[i];

	//임시메모리 해제 (alloca로 스택에 할당된 메모리는 자동으로 해제됨)
	//free(temp_pointers);
	//free(temp_keys);

	new_node->parent = left->parent;
	for (i = 0; i <= new_node->num_keys; i++) {
		child = new_node->pointers[i];
		child->parent = new_node;
	}

	//노드가 분리 되었으므로 부모에 다시 연결
	return _bpt_insert_into_parent (btree, left, kp, new_node);
}


// DELETION ---------------------------------------------------------------------------------------
/*
1.노드(리프,부모) 삭제
2.	노드 병합(합친후 삭제)
3.		노드가 가장 왼쪽에 있는가?
4.			리프 병합
5.			부모 병합
		노드 삭제(1번에서 반복)
6.	노드 재분배
7.		노드가 가장 왼쪽에 있는가?
8.			리프 재분배
9.			부모 재분배
*/

/*
1.bpt_delete ()
2.	bpt_delete_entry ()
3.		_bpt_remove_entry_from_node ()	//엔트리 삭제
4.		_bpt_adjust_root ()				//루트 노드 재조정
5.		_bpt_get_neighbor_index ()		//이웃(왼쪽) 노드 인덱스

		//이웃(왼쪽) 노드에 충분한 엔트리가 있으면 노드 병합(함침, 남는 노드는 삭제)
6.		_bpt_coalesce_nodes ()
			bpt_delete_entry () //2번으로 이동(반복)

		//이웃(왼쪽) 노드에 충분한 엔트리가 없으면(꽉차 있으면) 노드 재분배(키 이동)
7.		_bpt_redistribute_nodes ()
*/

///단독 B+트리 노드 삭제
NODE5* bpt_delete (BTREE* btree, void* key, bool *deleted)
{
	NODE5* leaf;
	int idx;
	void *pkey, *pointer;

	*deleted = false;
	//리프노드로 이동하여 key를 찾음
	if ((idx = bpt_find_leaf_key (btree, key, &leaf)) >= 0) {
		pkey = leaf->keys[idx];		//포인터(키)
		pointer = leaf->pointers[idx];	//포인터(데이터)

		//엔트리(배열)에서 키 삭제
		//키는 부모노드에도 있으므로 메모리 해제시 오류발생(여기서 메모리 누수 가능성 있음)
		//삭제되는 키가 부모노드에 없도록 _bpt_remove_entry_from_node 수정(2010-12-18)
		btree->root = bpt_delete_entry (btree, leaf, pkey, pointer, 0);	//key는 포인터키 아님(문자열)

		//메모리 해제
		free (pkey);
		free (pointer);

		btree->kno--;	//고유번호 감소
		btree->kcnt--;  //키 개수 감소
		*deleted = true;
	}
	return btree->root;
}

///노드안의 엔트리 삭제(반복 호출됨)
//중복키 때문에 flag 매개변수 추가(2011-01-17)
NODE5* bpt_delete_entry (BTREE* btree, NODE5* node, void* key, void* pointer, int flag)
{
	int		min_keys, max_keys;
	NODE5*	neighbor;
	int		neighbor_index;
	int		kp_index;
	void*	kp;

	//노드의 엔터리(배열)에서 키-포인터 삭제
	node = _bpt_remove_entry_from_node (btree, node, key, pointer, flag);

	//루트노드가 비어 있다면 재조정
	if (node == btree->root) return _bpt_adjust_root (btree->root);

	//노드에 최소한의 키들(개수)이 있다면 리턴(진행 멈춤)
	min_keys = node->is_leaf ? _bpt_half_order (btree->order - 1) : _bpt_half_order (btree->order) - 1;
	if (node->num_keys >= min_keys)	return btree->root;

	//노드에 최소한의 키들(개수)이 없다면, 노드 병합 및 재분배 진행

	//부모노드에서 현재 노드와 이웃(왼쪽)하는 노드 인덱스(-1일때는 현재노드가 가장 왼쪽노드)
	neighbor_index = _bpt_get_neighbor_index (node);
	kp_index = neighbor_index == -1 ? 0 : neighbor_index;
	//부모노드 키
	kp = node->parent->keys[kp_index];

	//현재 노드와 이웃하는 노드
	neighbor = neighbor_index == -1 ? node->parent->pointers[1] : node->parent->pointers[neighbor_index];

	max_keys = node->is_leaf ? btree->order : btree->order - 1;
	if (neighbor->num_keys + node->num_keys < max_keys)
		//이웃(왼쪽) 노드에 충분한 엔트리가 있으면 노드 병합(함침, 남는 노드 삭제)
		return _bpt_coalesce_nodes (btree, node, neighbor, neighbor_index, kp);
	else
		//이웃(왼쪽) 노드에 충분한 엔트리가 없으면(꽉차 있으면) 노드 재분배(키 이동)
		return _bpt_redistribute_nodes (btree->root, node, neighbor, neighbor_index, kp_index, kp);
}

//노드의 엔터리(배열)에서 키-포인터 삭제
//중복키 때문에 flag 매개변수 추가(2011-01-17)
NODE5* _bpt_remove_entry_from_node (BTREE* btree, NODE5* node, void* key, void* pointer, int flag)
{
	register int i, k, idx_end;
	void*	dkey;  //삭제되는 키
	NODE5*	node_tmp = NULL;

	i = 0;
	k = 0;

	if (flag) {  //중복키는 포인터로 비교
		while (key != node->keys[i] ) {
			i++;
			k++;
		}
	} else {
		while (btree->compare (key, node->keys[i]) ) {
			i++;
			k++;
		}
	}
	dkey = node->keys[i];
	//키 위치에서 왼쪽으로 이동(key 삭제)
	for (++i; i < node->num_keys; i++)
		node->keys[i - 1] = node->keys[i];

	//내부 노드는 포인터가 하나 많음(맨 오른쪽 포인터)
	idx_end = node->is_leaf ? node->num_keys : node->num_keys + 1;

	i = 0;
	while (node->pointers[i] != pointer) i++;
	//포인터 위치에서 왼쪽으로 이동(pointer 삭제)
	for (++i; i < idx_end; i++)
		node->pointers[i - 1] = node->pointers[i];

	//엔터리 개수 하나 감소
	node->num_keys--;

	//뒷부분 포인터는 NULL 처리
	if (node->is_leaf)
		for (i = idx_end; i < btree->order - 1; i++)  //리프노드는 맨 마지막 포인터 사용함
			node->pointers[i] = NULL;
	else
		for (i = idx_end; i < btree->order; i++)
			node->pointers[i] = NULL;

	//(2010-12-18, 추가): 리프의 제일 왼쪽 요소가 삭제 되었다면(k==0),
	//이것이 부모에 있으면 존비가 되므로 제거한다.(삭제된 키가 부모에 존재하는 것을 방지)
	if (k==0 && node->is_leaf) node_tmp = node->parent;
	while (node_tmp) {
		for (i = 0; i < node_tmp->num_keys; i++) {
			if (dkey == node_tmp->keys[i]) {	//포인터 비교
				node_tmp->keys[i] = node->keys[0];
				break;	//부모에 하나만 존재
			}
		} //for
		node_tmp = node_tmp->parent;
	} //while

	return node;
}

//루트노드가 비어 있다면 재조정
NODE5* _bpt_adjust_root (NODE5* root)
{
	NODE5* new_root;

	//비어 있지 않으면 그대로 반환
	if (root->num_keys > 0) return root;

	//비어 있으면
	if (root->is_leaf) {  //루트가 리프이면 자식 노드(포인터) 없음
		new_root = NULL;
	} else {  //루트가 리프가 아니라면 첫번째 자식 노드를 새로운 루트로
		new_root = root->pointers[0];
		new_root->parent = NULL;
	}

	//비어있는 루트노드 제거
	free (root->keys);
	free (root->pointers);
	free (root);

	//새로운 루트노드 반환
	return new_root;
}

//이웃(왼쪽)노드 인덱스
int _bpt_get_neighbor_index (NODE5* node)
{
	register int i;

	//현재 노드와 이웃하는 왼쪽노드 번호(-1일때는 현재노드가 가장 왼쪽노드)
	for (i = 0; i <= node->parent->num_keys; i++)
		if (node->parent->pointers[i] == node)
			return i - 1;

	//오류메세지 출력
	printf ("There is no left-child-node of parent in the _bpt_get_neighbor_index().\n");
	exit (EXIT_FAILURE);
}

//이웃(왼쪽) 노드에 충분한 엔트리가 있으면 노드 병합(함침, 남는 노드는 삭제)
NODE5* _bpt_coalesce_nodes (BTREE* btree, NODE5* node, NODE5* neighbor, int neighbor_index, void* kp)
{
	register int i, j;
	int idx, n_end;
	NODE5 *tmp;

	if (neighbor_index == -1) {  //현재 노드가 가장 왼쪽 노드일때, neighbor와 바꿈
		tmp = node;
		node = neighbor;
		neighbor = tmp;
	}

	//이웃 노드에 합쳐지는 위치 인덱스
	idx = neighbor->num_keys;

	if (node->is_leaf) {
	//현재 노드가 리프라면
		//현재노드를 이웃노드의 끝으로 이동(합침)
		for (i = idx, j = 0; j < node->num_keys; i++, j++) {
			neighbor->keys[i] = node->keys[j];
			neighbor->pointers[i] = node->pointers[j];
			neighbor->num_keys++;
		}
		neighbor->pointers[btree->order - 1] = node->pointers[btree->order - 1];

	} else {
	//현재 노드가 리프가 아니라면(내부노드)
		//부모노드의 키를 이웃의 끝으로 이동(내림)
		neighbor->keys[idx] = kp;
		neighbor->num_keys++;

		//현재 노드의 키수
		n_end = node->num_keys;
		//현재 노드를 이웃 노드의 끝으로 이동
		for (i = idx + 1, j = 0; j < n_end; i++, j++) {
			neighbor->keys[i] = node->keys[j];
			neighbor->pointers[i] = node->pointers[j];
			neighbor->num_keys++;
			node->num_keys--;
		} //for

		//포인터의 수는 키의 수보다 항상 하나 많다.
		neighbor->pointers[i] = node->pointers[j];

		//합쳐진 노드 자식들의 부모(neighbor->pointers->parent)는 합쳐진 노드(neighbor)가 되어야 한다.
		for (i = 0; i < neighbor->num_keys + 1; i++) {
			tmp = (NODE5 *)neighbor->pointers[i];
			tmp->parent = neighbor;
		} //for
	} //if

	//현재 노드의 부모 삭제(재호출)
	btree->root = bpt_delete_entry (btree, node->parent, kp, node, 1); //부모노드 키는 포인터 비교.

	//현재 노드 삭제
	free (node->keys);
	free (node->pointers);
	free (node);

	return btree->root;
}

//이웃(왼쪽) 노드에 충분한 엔트리가 없으면(꽉차 있으면) 노드 재분배
NODE5* _bpt_redistribute_nodes (NODE5* root, NODE5* node, NODE5* neighbor, int neighbor_index, int kp_index, void* kp)
{
	register int i;

	if (neighbor_index == -1) {
		//현재 노드가 가장 왼쪽노드이면(오른쪽 이웃에서 하나 가져옴)
		if (node->is_leaf) {
			//오른쪽 이웃노드의 가장 왼쪽에 있는 key-pointer 쌍을 왼쪽 노드의 마지막으로 옮김.
			node->keys[node->num_keys] = neighbor->keys[0];
			node->pointers[node->num_keys] = neighbor->pointers[0];
		} else {
			//부모노드의 키를 현재노드의 끝으로 이동(내림)
			node->keys[node->num_keys] = kp;
			//이웃 노드의 첫번째 포인터를 현재 노드의 끝으로 이동
			node->pointers[node->num_keys + 1] = neighbor->pointers[0];
			//이웃(neighbor) 노드의 부모는 현재(node) 노드가 됨
			((NODE5 *)node->pointers[node->num_keys + 1])->parent = node;

			//키를 부모노드로 이동
			node->parent->keys[kp_index] = neighbor->keys[0];
		}
		//오른쪽 이웃노드의 키-포인터를 한칸씩 왼쪽으로 이동
		for (i = 0; i < neighbor->num_keys; i++) {
			neighbor->keys[i] = neighbor->keys[i + 1];
			neighbor->pointers[i] = neighbor->pointers[i + 1];
		}
		if (node->is_leaf)
			node->parent->keys[kp_index] = neighbor->keys[0];  //키를 부모노드로 이동
		else
			neighbor->pointers[i] = neighbor->pointers[i + 1];

	} else {
	//왼쪽 이웃 노드에서 하나 가져옴
		if (!node->is_leaf)
			node->pointers[node->num_keys + 1] = node->pointers[node->num_keys];
		//오른쪽으로 이동
		for (i = node->num_keys; i > 0; i--) {
			node->keys[i] = node->keys[i - 1];
			node->pointers[i] = node->pointers[i - 1];
		}
		if (node->is_leaf) {
			node->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
			neighbor->pointers[neighbor->num_keys - 1] = NULL;
			node->keys[0] = neighbor->keys[neighbor->num_keys - 1];
			//키를 부모노드로 이동
			node->parent->keys[kp_index] = node->keys[0];
		} else {
			node->pointers[0] = neighbor->pointers[neighbor->num_keys];
			((NODE5 *)node->pointers[0])->parent = node;
			neighbor->pointers[neighbor->num_keys] = NULL;
			node->keys[0] = kp;
			node->parent->keys[kp_index] = neighbor->keys[neighbor->num_keys - 1];
		}
	}
	//현재노드 키 증가
	node->num_keys++;
	//이웃노드 키 감소
	neighbor->num_keys--;

	return root;
}


// DESTROY ----------------------------------------------------------------------------------------

//리프노드에 할당된 메모리와 노드 포인터들을 동시에 해제(재귀호출)
unsigned int bpt_drop_leaves_nodes (BTREE* btree, NODE5* node)
{
	register int i;
	char prompt[] = {'+', '-', '*'};
	static unsigned int cnt = 0;

	if (!node) {
		printf("** Empty.\n");
		return 0;
	}

	if (node->is_leaf) {
		for (i = 0; i < node->num_keys; i++) {
			free (node->pointers[i]);
			btree->kcnt--;
		}
	} else {
		for (i = 0; i < node->num_keys + 1; i++)
			bpt_drop_leaves_nodes (btree, node->pointers[i]);  //재귀호출
	}

	free (node->pointers);
	free (node->keys);
	free (node);

	cnt++; //노드들의 개수
	printf ("%c(%c)%u ", CR, prompt[cnt%3], cnt);  //CR, 프롬프트, 반복수

	return cnt;
}

//리프노드에 할당된 메모리 해제
unsigned int bpt_drop_leaves (BTREE* btree, NODE5* node)
{
	register int i;
	unsigned int cnt = 0;
	char prompt[] = {'+', '-', '*'};

	if (!node) {
		printf("** Empty.\n");
		return cnt;
	}
	//리프 노드에 할당된 메모리 해제
	while (!node->is_leaf)
		node = (NODE5*)node->pointers[0];  //트리 높이만큼 반복

	while (true) {
		cnt += node->num_keys;
		for (i = 0; i < node->num_keys; i++) {
			free (node->keys[i]);
			free (node->pointers[i]);
			btree->kcnt--;
		}
		printf ("%c(%c)%u ", CR, prompt[cnt%3], cnt);  //CR, 프롬프트, 반복수

		if (node->pointers[btree->order - 1])
			node = (NODE5*)node->pointers[btree->order - 1];	//다음 리프 노드
		else break;
	} //while

	return cnt; //리프에 있는 key 개수
}

//노드 포인터 해제(재귀호출)
unsigned int bpt_drop_nodes (NODE5* node)
{
	register int i;
	char prompt[] = {'+', '-', '*'};
	static unsigned int cnt = 0;

	if (!node) {
		printf("** Empty.\n");
		cnt = 0;
		return cnt;
	}

	if (!node->is_leaf) {
		for (i = 0; i < node->num_keys + 1; i++)
			bpt_drop_nodes (node->pointers[i]); //노드 포인터를 재귀호출
	}

	free (node->pointers);
	free (node->keys);
	free (node);

	cnt++;
	printf ("%c(%c)%u ", CR, prompt[cnt%3], cnt);  //CR, 프롬프트, 반복수

	return cnt; //노드 개수
}


//B+ 트리 모든 노드 삭제
void bpt_drop (BTREE** ws, BTREE** wi)
{
	unsigned int cnt;

	cnt = bpt_drop_leaves_nodes (*ws, (*ws)->root);
	(*ws)->root = NULL;
	printf ("%c(-)%u s-nodes have removed.", CR, cnt);
	(*ws)->kno = (*ws)->kcnt;	//0

	cnt = bpt_drop_leaves_nodes (*wi, (*wi)->root);
	(*wi)->root = NULL;
	printf ("%c(-)%u i-nodes have removed.\n", CR, cnt);
	(*wi)->kno = (*wi)->kcnt;	//0

	/*
	//리프노드에 할당된 메모리 해제 (ws와 wi는 key,pointer 공유)
	cnt = bpt_drop_leaves (*ws, (*ws)->root);
	printf ("%c(-)%u data leaves have removed.\n", CR, cnt);

	cnt = bpt_drop_nodes ((*ws)->root);		//ws 노드 포인터 해제
	(*ws)->root = NULL;
	(*ws)->kno = (*ws)->kcnt;	//0
	printf ("%c(-)%u s-nodes have removed.\n", CR, cnt);

	cnt = bpt_drop_nodes ((*wi)->root);		//wi 노드 포인터 해제
	(*wi)->root = NULL;
	(*wi)->kcnt = (*ws)->kcnt;	//0
	(*wi)->kno = (*wi)->kcnt;	//0
	printf ("%c(-)%u i-nodes have removed.\n", CR, cnt);
	*/
}

//B+ 트리 모든 노드와 B+트리까지 삭제
void bpt_drop_all (BTREE** ws, BTREE** wi)
{
	bpt_drop (ws, wi);
	free (*ws);
	free (*wi);
	*ws = NULL;
	*wi = NULL;
}
