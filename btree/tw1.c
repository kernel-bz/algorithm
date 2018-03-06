//
//  Source: tw1.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-12-06 tw1.c, 번역 메인 모듈을 코딩하다.
//		2010-12-11 B+Tree에 랜덤 키를 약 40억번 삽입 및 삭제하여 무결성을 점검하다.
//		2010-12-17 문자열키와 숫자키를 작업할 수 있도록 B+Tree를 각각 생성하다.
//		2010-12-18 키 입력 및 삭제시 메모리 해제되지 않는 문제 수정하다.
//		2010-12-21 파일 입출력(fileio) 함수들을 추가하다.
//		2010-12-25 번역논리를 코딩하다.
//		2011-01-14 번역용 숫자키 삭제함수(_tw1_delete_trans_key) 추가하다. --> V1.2011.01
//		2011-01-18 번역키 비교함수에서 중복키를 허용하다.
//		2011-01-26 단어개수에 따라서 색인키를 2개로 분배하다.
//		2011-01-28 문자부호(! " , ? : ; `) 처리 기능을 추가하다.(is_space_mark 함수)
//		2011-02-09 교정단어 보정기능 추가하고 문장입력을 하나의 함수(tw1_words_input)로 통합하다.
//		2011-02-11 단어 삭제시 고유번호 관리용 큐(QUEUE* qk[])를 추가하다.
//		2011-02-11 tw2.c 모듈을 분리하여 코딩 시작하다.
//		yyyy-mm-dd ...
//

#include <stdlib.h>
#include <stdio.h>

#include "dtype.h"
#include "umac.h"
#include "ustr.h"
#include "utime.h"
#include "fileio.h"
#include "bpt3.h"
#include "queue.h"
#include "stack.h"
#include "tw1.h"
#include "tw2.h"

//alloca() 함수 헤더
#ifdef __LINUX
	#include <alloca.h>
	#include "umem.h"
#else
	#include <malloc.h>
#endif


int tw1_compare_int (void* p1, void* p2)
{
	if (*(unsigned int*)p1 <  *(unsigned int*)p2) return -1;
	if (*(unsigned int*)p1 == *(unsigned int*)p2) return 0;
	return 1;
}

int tw1_compare_str (void* p1, void* p2)
{
	return str_cmp ((char *)p1, (char *)p2);
}

//숫자키 문자열을 unsigned int로 변환후 비교
int tw1_compare_str_int (void* p1, void* p2)
{
	return str_cmp_int ((char *)p1, (char *)p2);
}

void tw1_output_int (void* p1)
{
	printf ("%u", *(unsigned int*)p1);
}

void tw1_output_str (void* p1)
{
	printf ("%s", (char*)p1);
}

//질문(Yes/No)에 대한 답변값
int tw1_qn_answer (char *msg, int ans)
{
	register int i=0;
	char c, str[ASIZE];

	printf ("%s", msg);
	while ((c = getchar()) != (int)'\n') str[i++] = a_lower (c);

	if (*str == 'y') ans = FLAG_YES;
	else if (*str == 'n') ans = FLAG_NO;

	printf ("\n");
	return ans;
}

//단어 입력후 숫자키 반환
unsigned int _tw1_insert_to_btree (BTREE* ws, BTREE* wi, char *in_word, QUEUE* que_kno)
{
	NODE5	*leaf_ps, *leaf_pi;
	char*	pword;
	unsigned int *pno;  //입력용 일련번호(고유숫자)
	int		idx, err=0, flag;
	bool	is_qk;

	if (! *in_word) return UIFAIL;

	if (ws->kno != wi->kno)	{	//인덱스 고유번호 다름(이조건이 발생하면 않됨)
		printf ("## Failure to index serial number.(ws:%u, wi:%u)\n", ws->kno, wi->kno);
		return UIFAIL;
	}

	idx = bpt_find_leaf_key (ws, in_word, &leaf_ps);
	if (idx >= 0) return *(unsigned int*)leaf_ps->pointers[idx];	///문자 키 존재

	///리프에 동일한 키가 없다면 메모리할당후 삽입
	pword = malloc (str_len(in_word) + 1);
	if (!pword) {
		printf ("## Failure to allocate memory.\n");
		err++;  //메모리 할당 실패
	}
	str_copy (pword, in_word);
	pno = malloc (sizeof(unsigned int));
	if (!pno) {
		printf ("## Failure to allocate memory.\n");
		err++;  //메모리 할당 실패
	}

	///큐에 보관된 고유번호가 있으면 가져옴
	is_qk = que_dequeue (que_kno, (void*)&pno);
	if (!is_qk)	{
		*pno = ws->kno;  ///큐에 없다면 순번 사용(삽입시 증가됨)
		if (UIFAIL == *pno + 1) {
			printf ("## Key serial number(kno) exceed!!\n");
			err++;
		}
	}
	if (bpt_find_leaf_key (wi, pno, &leaf_pi) >= 0) {
		///고유번호 존재 (이조건이 발생하면 않됨)
		printf ("## Exist same number key.(s:%s, i:%u)\n", in_word, *pno);
		err++;
	}

	if (err) {
		free (pword);
		free (pno);
		return UIFAIL;
	}

	///큐에 있는 고유번호를 사용했다면 kno 증가않됨(FLAG_UPDATE)
	flag = (is_qk) ? FLAG_UPDATE : FLAG_INSERT;

	///문자키 입력
	ws->root = bpt_insert (ws, leaf_ps, pword, pno, flag);

	///순자키 입력
	wi->root = bpt_insert (wi, leaf_pi, pno, pword, flag);

	return *pno;
}

//번역용 입력
bool _tw1_insert_to_btree_trans (BTREE* pts1, BTREE* pts2, char* keys1, char* keys2)
{
	NODE5 *leaf, *leaf2;
	char  *pw1, *pw2;

	if (bpt_find_leaf_key (pts1, keys1, &leaf) >= 0) {	//첫번째 키 존재
		if (bpt_find_leaf_key (pts2, keys2, &leaf2) >= 0) {	//두번째 키 존재
			//printf ("** Exist same sentence.\n");
			return false;	//동시 중복 회피
		}
	}

	//pts1과 pts2의 키들중에 새로운 키를 삽입해야 한다면, 메모리 할당후 삽입
	pw1 = malloc (str_len (keys1) + 1);
	if (!pw1) {
		printf ("## Failure to allocate memory.\n");
		return false;  //메모리 할당 실패
	}
	str_copy (pw1, keys1);

	pw2 = malloc (str_len (keys2) + 1);
	if (!pw2) {
		printf ("## Failure to allocate memory.\n");
		return false;  //메모리 할당 실패
	}
	str_copy (pw2, keys2);

	//번역용 pts1에 삽입
	pts1->root = bpt_insert (pts1, leaf, pw1, pw2, FLAG_INSERT);

	leaf2 = bpt_find_leaf (pts2, pw2, pts2->compare);
	//번역용 pts2에 삽입
	pts2->root = bpt_insert (pts2, leaf2, pw2, pw1, FLAG_INSERT);

	return true;
}

//단어 키 삭제
unsigned int tw1_drop_word_run (BTREE* ws, BTREE* wi, char *key, bool *deleted)
{
	void *ptr1, *ptr2;	//넘겨 받는 포인터
	unsigned int pno;
	int idx;
	NODE5 *leaf1, *leaf2;

	*deleted = false;
	if ((idx = bpt_find_leaf_key (ws, key, &leaf1)) >= 0) {
		ptr1 = leaf1->keys[idx];		//포인터(키)
		ptr2 = leaf1->pointers[idx];	//포인터(데이터)
		pno = *((unsigned int*)ptr2);

		if ((idx = bpt_find_leaf_key (wi, ptr2, &leaf2)) >= 0) {	//숫자 포인터
			//ws 엔트리(배열)에서 키 삭제
			ws->root = bpt_delete_entry (ws, leaf1, ptr1, ptr2, FLAG_NONE);	//flag:FLAG_NONE(중복키 없음)
			//ws->kno--;	//고유번호는 큐에 저장 되므로 감소하지 않음
			ws->kcnt--;		//키 개수 감소

			//wi 엔트리(배열)에서 키 삭제
			wi->root = bpt_delete_entry (wi, leaf2, ptr2, ptr1, FLAG_NONE);	//flag:FLAG_NONE(중복키 없음)
			//wi->kno--;	//고유번호는 큐에 저장 되므로 감소하지 않음
			wi->kcnt--;		//키 개수 감소

			//문자키 포인터 메모리 해제
			free (ptr1);
			//숫자키 포인터 메모리 해제
			free (ptr2);

			*deleted = true;
			return pno;		//삭제된 키 고유번호

		} else {
			printf ("## Don't find pointer key in the deletion.(%s, %u)\n", key, pno);
			bpt_find_leaf_debug (wi, ptr2, true);
			return UIFAIL;
		}
	}
	return 0;	//UIFAIL;	//삭제대상 없음(unsigned int 최대값)
}

//번역 키 삭제(중복된 키 때문에 bpt_delete_entry에서 포인터로 비교)
int _tw1_delete_trans_key_run (BTREE* pt1, BTREE* pt2, NODE5* leaf, int k)
{
	void  *ptr1, *ptr2;		//넘겨 받는 포인터
	NODE5 *leaf2;	//넘겨 받는 리프
	double msec1, msec2;

	ptr1 = leaf->keys[k];		//포인터(키)
	ptr2 = leaf->pointers[k];	//포인터(데이터)

	//미리 초 단위(시작)
	msec1 = time_get_msec ();

	if (bpt_find_leaf_key_trans (pt2, ptr2, ptr1, &leaf2) >= 0) {
		//pt1 엔트리(배열)에서 키 삭제
		pt1->root = bpt_delete_entry (pt1, leaf, ptr1, ptr2, FLAG_YES);	//flag:FLAG_YES(포인터 키 비교, 중복키 때문)
		pt1->kno--;
		pt1->kcnt--;  //키 개수 감소

		//pt2 엔트리(배열)에서 키 삭제
		pt2->root = bpt_delete_entry (pt2, leaf2, ptr2, ptr1, FLAG_YES);	//flag:FLAG_YES(포인터 키 비교, 중복키 때문)
		pt2->kno--;
		pt2->kcnt--;  //키 개수 감소

		//pt1의 키 메모리 해제
		free (ptr1);
		//pt2의 키 메모리 해제
		free (ptr2);

		msec2 = time_get_msec ();
		//실행시간
		printf ("** Deletion Time: %.3f Secs\n\n", msec2 - msec1);

		return 1;

	} else {
		printf ("## Don't find pointer key in the deletion.\n");
		bpt_find_leaf_debug (pt2, ptr2, true);
		return -1;	//UIFAIL
	}
}

///입력작업(직접 키보드 입력)
int tw1_insertion (BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk)
{
	char keys[2][SSIZE];	//숫자조합 번역키 배열
	NODE5 *leaf;
	BTREE *t1, *t2;		//번역용 인덱스
	int cnt, isave=0, idx;
	unsigned int h;
	double msec1, msec2;

	do {
		//미리 초 단위(시작)
		msec1 = time_get_msec ();

		if ((cnt = tw1_words_input (_tw1_prompt(mode), keys[mode], ws[mode], wi[mode], rs, qk[mode], FLAG_INSERT)) > 0) {
			//해시값으로 번역키 B+트리 선택
			h = hash_value (keys[mode]);
			t1 = hb[mode][h];

			//이미 번역된 것이 있다면 출력
			if ((idx = bpt_find_leaf_key (t1, keys[mode], &leaf)) >= 0)
				isave = _tw1_trans_key_finder (leaf, idx, keys[mode], t1, hb[!mode], wi[!mode], FLAG_INSERT);	//동일한 키가 있다면 출력

			if ((cnt = tw1_words_input (_tw1_prompt(!mode), keys[!mode], ws[!mode], wi[!mode], rs, qk[!mode], FLAG_INSERT)) > 0) {
				//미리 초 단위(시작)
				msec1 = time_get_msec ();

				h = hash_value (keys[!mode]);
				t2 = hb[!mode][h];

				//번역용 B+트리에 저장
				if (_tw1_insert_to_btree_trans (t1, t2, keys[mode], keys[!mode])) isave++;
			}
		} //if
		//미리 초 단위(종료)
		msec2 = time_get_msec ();
		//실행시간
		printf ("** Insertion Time: %.3f Secs\n\n", msec2 - msec1);

	} while (cnt > 0);

	return isave;
}

//입력작업(파일에서 자동 입력)
//int sh: 정해진 해시값 (음수이면 해시값 정하지 않음)
int tw1_insertion_from_file (BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, char rows[2][SSIZE], int sh)
{
	char keys[2][SSIZE];	//숫자조합 번역키 배열
	//NODE5 *leaf;
	BTREE *t1, *t2;		//번역용 인덱스
	int saved = 0, idx;
	unsigned int h;

	str_copy (keys[mode], rows[mode]);	//행단위 문자열 문장1

	//숫자키로 변환
	if ((idx = tw1_words_input (_tw1_prompt(mode), keys[mode], ws[mode], wi[mode], rs, qk[mode], FLAG_AUTO)) > 0) {
		//키값으로 해시값 얻어서 B+트리 선택
		if (sh < 0) {
			h = hash_value (keys[mode]);
			t1 = hb[mode][h];
		} else t1 = hb[mode][sh];

		//동일한 키가 있는지 여부
		//if ((idx = bpt_find_leaf_key (t1, keys[mode], &leaf)) < 0)	{
			str_copy (keys[!mode], rows[!mode]);	//행단위 문자열 문장2

			//번역키로 변환 성공 여부
			if ((idx = tw1_words_input (_tw1_prompt(!mode), keys[!mode], ws[!mode], wi[!mode], rs, qk[!mode], FLAG_AUTO)) > 0) {
				if (sh < 0) {
					h = hash_value (keys[!mode]);
					t2 = hb[!mode][h];
				} else t2 = hb[!mode][sh];
				//동일한 번역키가 없을때 (중복회피)
				if (_tw1_insert_to_btree_trans (t1, t2, keys[mode], keys[!mode])) {
					saved = 1;
					//printf ("%s\n", rows[mode]);
					//printf (">> %s\n", rows[!mode]);
				}
			}
		//}
	}
	return saved;
}

//숫자키들을 검색하여 번역문자 출력
int _tw1_trans_key_data (BTREE* wi, void* keys, int kcnt)
{
	char	*ckeys, adigit[ASIZE], buf[SSIZE];
	void	*data;
	register int i;
	int		cnt = 0;
	unsigned int *pkey;

	//pkey = malloc (sizeof(unsigned int));
	//Win: <malloc.h>
	//Linux: <alloca.h> 스택에 메모리를 빠르게 할당, scope를 벗어나면 자동해제 되므로 free할 필요없음.
	pkey = alloca (sizeof(unsigned int));
	if (!pkey) {
		printf ("## Failure to allocate alloca in _tw1_trans_key_data().\n");
		return 0;  //메모리 할당 실패
	}

	buf[0] = '\0';
	ckeys = (char*)keys;
	while (*ckeys) {
		i = 0;
		while ( (adigit[i++] = *ckeys++) != '_');
		i--;
		if (kcnt-- > 0) continue;	//유사한 문장 검색어(caption) 제외
		str_cat (buf, " ");	//단어단위 한칸 뛰움

		adigit[i] = '\0';
		*pkey = str_to_uint (adigit);
		data = bpt_search (wi, pkey);
		if (data) {
			cnt++;
			if (cnt == 1) {
				*(char*)data = a_upper (*(char*)data);	//첫단어의 시작문자는 대문자로
				//wi->outdata (data);
				str_cat (buf, data);
				*(char*)data = a_lower (*(char*)data);	//소문자로 다시 되돌림
			} else {
				//wi->outdata (data);
				str_cat (buf, data);
			}
		} else str_cat (buf, "~");  //없음
	}
	//free (pkey);  //alloca 에서 스택에 할당된 메모리는 scope를 벗어나면 자동으로 해제됨

	if (cnt) {
		str_cat (buf, ".\n");
		printf ("%s", buf);
		//번역 문장 보관용 스택에 입력
		tw2_stack_push (buf);
	}
	return cnt;
}

//_tw1_trans_key_data() 함수와 내용은 동일함
int _tw1_trans_key_data_each (BTREE* wi, void* keys)
{
	char	*ckeys, adigit[ASIZE];  //단어길이
	void	*data;
	register int i;
	int		cnt = 0;
	unsigned int *pkey;

	pkey = alloca (sizeof(unsigned int));	//스택에 메모리 할당 (자동해제됨)
	if (!pkey) {
		printf ("## Failure to allocate alloca in _tw1_trans_key_data().\n");
		return 0;  //메모리 할당 실패
	}

	ckeys = (char*)keys;
	while (*ckeys) {
		i = 0;
		while ( (adigit[i++] = *ckeys++) != '_');
		i--;
		printf (" ");

		adigit[i] = '\0';
		*pkey = str_to_uint (adigit);
		data = bpt_search (wi, pkey);
		if (data) {
			cnt++;
			if (cnt == 1) {
				*(char*)data = a_upper (*(char*)data);	//첫단어의 시작문자는 대문자로
				wi->outdata (data);
				*(char*)data = a_lower (*(char*)data);	//소문자로 다시 되돌림
			} else wi->outdata (data);
		} else printf ("~");  //없음
	}
	//free (pkey);  //alloca 에서 스택에 할당된 메모리는 scope를 벗어나면 자동으로 해제됨
	return cnt;
}

//동일한 숫자키(모두일치)가 더 있으면 번역
int _tw1_trans_key_finder (NODE5* leaf, int k, char *keys, BTREE* pt1, BTREE* hb[], BTREE* pi_trg, int flag)
{
	register int isave=0, cnt=1;
	BTREE* pt2;

	do {
		//번역
		if (_tw1_trans_key_data (pi_trg, leaf->pointers[k], 0) == 0) break;

		pt2 = hb[hash_value (leaf->pointers[k])];	//pt를 해시값으로 다시 설정

		//동일키 삭제되면 leaf->num_keys 줄어듬
		if (flag) {
			if (tw1_qn_answer ("* Would you like to delete this? [y/N] ", FLAG_NO) == FLAG_YES) {
				isave = _tw1_delete_trans_key_run (pt1, pt2, leaf, k);
				cnt -= isave;
			}
		} else {
			isave = _tw1_delete_trans_key_run (pt1, pt2, leaf, k);
			cnt -= isave;
		}
		cnt++;
		//idx = tw1_qn_answer ("\n** Would you like to see more? [Y/n] ", FLAG_YES);  //FLAG_NO(-1)
	} while (isave >= 0 && (k = bpt_find_leaf_key_next (pt1, keys, &leaf, cnt)) >= 0);

	return (isave > 0) ? 1 : 0;
}

//동일한 키(모두일치) 번역
void _tw1_trans_key_equal (NODE5* leaf, char *keys, int k, BTREE* pt1, BTREE* pi_trg)
{
	register int i, idx;

	idx = -1;
	for (i = k; i < leaf->num_keys; i++) {
		idx = pt1->compare (keys, leaf->keys[i]);
		if (idx == 0)	//동일한 키가 있다면 번역
			_tw1_trans_key_data (pi_trg, leaf->pointers[i], 0);
		else break;
	} //for

	//다음 리프노드에도 동일한 키가 있다면 번역
	while (idx == 0) {
		if (leaf->pointers [pt1->order - 1]) {
			leaf = leaf->pointers [pt1->order - 1];
			for (i = 0; i < leaf->num_keys; i++) {
				idx = pt1->compare (keys, leaf->keys[i]);
				if (idx == 0) {
					if (i == leaf->num_keys-1)
						idx = tw1_qn_answer ("* Would you like to see more? [Y/n] ", FLAG_YES) - 1;  //FLAG_YES(1), FLAG_NO(-1)
					if (idx) break;
					_tw1_trans_key_data (pi_trg, leaf->pointers[i], 0);	//동일한 키가 있다면 번역
				} else break;
			} //for
		} else idx = -1;
	} //while
}

//앞부분 일치 키가 있다면 번역(range scan)
int _tw1_trans_key_like (char *keys, BTREE* pt, BTREE* pi_src, BTREE* pi_trg, int kcnt)
{
	NODE5* leaf;
	register int i;
	int idx=0, ifind=0;

	i = bpt_find_leaf_key_like (pt, keys, &leaf);	//유사한 키를 리프에서 한번 더 찾음
	if (i < 0) return 0;

	do {
		//문장으로 변환
		if (_tw1_trans_key_data (pi_src, leaf->keys[i], kcnt)) {
			_tw1_trans_key_data (pi_trg, leaf->pointers[i], kcnt);	//번역
			printf ("\n");
			ifind++;
		}
	} while ( (++i < leaf->num_keys) &&
		      (idx = str_cmp_int_like (keys, leaf->keys[i])) == 0 );	//키까지만 비교(왼쪽 앞부분 일치)

	//다음 리프노드에도 유사한 키가 있다면 번역
	while (idx == 0) {
		if (leaf->pointers [pt->order - 1]) {
			leaf = leaf->pointers [pt->order - 1];
			for (i = 0; i < leaf->num_keys; i++) {
				idx = str_cmp_int_like (keys, leaf->keys[i]);
				if (idx == 0) {
					//문장으로 변환
					if (_tw1_trans_key_data (pi_src, leaf->keys[i], kcnt)) {
						_tw1_trans_key_data (pi_trg, leaf->pointers[i], kcnt);	//번역
						ifind++;
						printf ("\n");

						if (!(ifind%pt->order))
							idx = tw1_qn_answer ("* Would you like to see more? [Y/n] ", FLAG_YES) - 1;
						if (idx) break;
					}
				} else break;
			} //for
		} else idx = -1;
	} //while

	//return ifind;
	return (idx == -2) ? idx : ifind;	//-2: 사용자 중지
}

//유사한 번역키 검색(리프노드 전체검색: full scan)
int _tw1_trans_key_similar (char *keys, BTREE* pt, BTREE* pi_src, BTREE* pi_trg)
{
	NODE5* leaf;
	register int i;
	int idx=0, ifind=0;

	i = bpt_find_leaf_key_similar (pt, keys, &leaf);	//유사한 키를 리프에서 한번 더 찾음
	if (i < 0) return 0;

	do {
		printf ("\n\n");
		_tw1_trans_key_data (pi_src, leaf->keys[i], 0);		//유사한 문장
		_tw1_trans_key_data (pi_trg, leaf->pointers[i], 0);	//번역
		ifind++;
	} while ( (++i < leaf->num_keys) &&
		      (idx = str_cmp_int_similar (keys, leaf->keys[i])) == 0 );	//키까지만 비교

	//다음 리프노드에도 유사한 키가 있다면 번역
	while (idx == 0) {
		if (leaf->pointers [pt->order - 1]) {
			leaf = leaf->pointers [pt->order - 1];
			for (i = 0; i < leaf->num_keys; i++) {
				idx = str_cmp_int_similar (keys, leaf->keys[i]);
				if (idx == 0) {
					if (i == leaf->num_keys-1)
						idx = tw1_qn_answer ("* Would you like to see more? [Y/n] ", FLAG_YES) - 1;  //FLAG_YES(1), FLAG_NO(-1)
					if (idx) break;

					printf ("\n");
					_tw1_trans_key_data (pi_src, leaf->keys[i], 0);		//유사한 문장
					_tw1_trans_key_data (pi_trg, leaf->pointers[i], 0);	//번역
					ifind++;
				} else break;
			} //for
		} else idx = -1;
	} //while

	//return ifind;
	return (idx == -2) ? idx : ifind;	//-2: 사용자 중지
}

//키를 하나씩 분리하여 단어 단위로 검색
int _tw1_trans_key_each (char* keys, BTREE** hb[], BTREE** wi, int mode, int kcnt)
{
	int		i=0, cnt=0, idx;
	unsigned int h, *akey2;
	BTREE	*t1;
	NODE5*	leaf;
	char	akey[ASIZE], *pkey;

	akey2 = alloca (sizeof(unsigned int)); //스택에 할당(자동해제됨)
	if (!akey2) {
		printf ("## Failure to allocate alloca in _tw1_trans_search_each().\n");
		return -1;  //메모리 할당 실패
	}

	printf (" *");
	while (i < kcnt) {
		pkey = akey;
		while (*keys != '_') *pkey++ = *keys++;
		*pkey++ = *keys++;
		*pkey = '\0';
		i++;

		//해시값으로 번역키 B+트리 선택
		h = hash_value (akey);
		t1 = hb[mode][h];
		if ((idx = bpt_find_leaf_key (t1, akey, &leaf)) >= 0) {
			_tw1_trans_key_data_each (wi[!mode], leaf->pointers[idx]);
			cnt++;
		} else printf (" -");	//키값 없음
		printf ("/");
	} //while
	printf ("\n\n");
	return cnt;
}

//번역을 위한 검색, keys 인덱스 --> hb(번역용 인덱스) --> wi(숫자열)
//완전일치 번역키가 없으면 HASHSIZE 만큼 B+트리 탐색을 반복하므로 속도 저하 있음
int _tw1_trans_search (char* keys, BTREE** hb[], BTREE** wi, BTREE* rs, int mode, int kcnt)
{
	register int i;
	unsigned int h;
	int		idx=0, cnt=0, cnt2=0, cnt3=0;
	BTREE	*t1, *t2;
	NODE5*	leaf;
	char*	cap;

	if (kcnt <= 0) return -1;

	//해시값으로 번역키 B+트리 선택
	h = hash_value (keys);
	t1 = hb[mode][h];

	if ((idx = bpt_find_leaf_key (t1, keys, &leaf)) >= 0) {
		//완전 일치키가 있다면 번역
		_tw1_trans_key_equal (leaf, keys, idx, t1, wi[!mode]);
		printf ("\n");
		cnt = 1;
	}

	//완전일치 번역이 없다면, 키를 하나씩 분리하여 단어 단위로 검색
	if (cnt==0 && kcnt > 1)
		_tw1_trans_key_each (keys, hb, wi, mode, kcnt);


	//CAPTION 검색(앞부분 캡션 일치키 like% 검색)
	cap = bpt_search (rs, keys);
	if (cap) {	//캡션이 있다면
		//rs->outdata (cap);
		_tw1_trans_key_data (wi[!mode], cap, 0);
		printf ("\n");

		h = 0;	//캡션 문장은 해시값을 0으로 정함.
		t2 = hb[mode][h];
		idx = _tw1_trans_key_like (keys, t2, wi[mode], wi[!mode], kcnt+2);
		cnt += (idx < 0) ? 0 : idx;	//검색개수
	}

	//검색결과가 없다면, 앞부분 일치키(like%)가 검색될때 까지 해시테이블의 B+트리 계속 탐색
	if (cnt==0) {
		for (i = 1; i < HASHSIZE; i++) {
			t2 = hb[mode][i];
			idx = _tw1_trans_key_like (keys, t2, wi[mode], wi[!mode], 0);
			cnt2 += (idx < 0) ? 0 : idx;	//검색개수
			if (idx < 0) break;	//사용자 중지
			if ((cnt3 < cnt2/10) && tw1_qn_answer ("* Would you like to see more? [Y/n] ", FLAG_YES) == FLAG_NO) break;
			cnt3 = cnt2 / 10;
		}
	}

	//검색결과가 없다면, 키가 중간에 포함되어 있는 문장 검색 (리프노드 전체 스캔)
	if (!cap && !cnt2 && kcnt < SIMILAR) {
		for (i = 1; i < HASHSIZE; i++) {
			t2 = hb[mode][i];
			idx = _tw1_trans_key_similar (keys, t2, wi[mode], wi[!mode]);
			cnt += (idx < 0) ? 0 : idx;	//검색개수
			if (idx) break;	//검색됨 혹은 사용자중지
		}
	}
	printf ("\n");
	return cnt + cnt2;
}

//번역 (문자열키, 번역키, 숫자조합키(target, source))
void tw1_translation (BTREE** ws, BTREE** hb[], BTREE** wi, int mode, char *prompt, BTREE* rs, QUEUE** qk)
{
	char	keys[SSIZE];
	int		kcnt;	//번역키 조합개수
	double	msec1, msec2;

	if (!ws[mode]->root) {
		printf("** Empty.\n");
		return;
	}
	while (true) {
		//문자를 입력받아서 검색한 인덱스를 keys에 저장
		kcnt = tw1_words_input (prompt, keys, ws[mode], wi[mode], rs, qk[mode], FLAG_TRANS);
		if (kcnt < 0) break;	//입력문자가 없을때(그냥 엔터)

		//미리 초 단위(시작)
		msec1 = time_get_msec ();

		if (_tw1_trans_search (keys, hb, wi, rs, mode, kcnt) <= 0) {
			printf ("** This words does not exist.\n");
			//스택에 저장된 번역대상 문장 제거
			tw2_stack_pop ();
		}

		//미리 초 단위(종료)
		msec2 = time_get_msec ();
		//실행시간
		printf ("** Translation Time: %.3f Secs\n\n", msec2 - msec1);
	}
}

//번역용 숫자키 삭제
int _tw1_delete_trans_key (char* keys, int cnt, BTREE** hb[], BTREE** wi, int mode)
{
	int		isave=0, idx;
	unsigned int h;
	NODE5	*leaf;
	BTREE	*t1;

	//해시값으로 번역키 B+트리 선택
	h = hash_value (keys);
	t1 = hb[mode][h];

	//동일한 키가 있다면 삭제
	if ((idx = bpt_find_leaf_key (t1, keys, &leaf)) >= 0)
		isave = _tw1_trans_key_finder (leaf, idx, keys, t1, hb[!mode], wi[!mode], FLAG_DELETE);

	return isave;
}

///번역용 숫자키 삭제
int tw1_deletion (BTREE** ws, BTREE** hb[], BTREE** wi, int mode, char* prompt, BTREE* rs, QUEUE** qk)
{
	char keys[SSIZE];
	int  cnt, isave=0;

	if (!ws[mode]->root) {
		printf("** Empty.\n");
		return 0;
	}

	//문자를 입력받아서 검색한 인덱스를 keys에 저장
	cnt = tw1_words_input (prompt, keys, ws[mode], wi[mode], rs, qk[mode], FLAG_DELETE);

	//번역용 인덱스 삭제
	if (cnt > 0) {
		isave = _tw1_delete_trans_key (keys, cnt, hb, wi, mode);
		if (! isave) printf ("** Not Exist!\n");
	}
	return isave;
}

//단어삭제 (단어사전에 오직 하나 남아 있을때, 필요없는 단어일때 사용)
//단어 삭제하고 고유번호를 큐에 저장(재사용 위해)
bool tw1_drop_word (BTREE* ws, BTREE* wi, QUEUE* queue)
{
	char *pstr, words[SSIZE];  //문장길이
	char aword[ASIZE];  //단어길이
	register int i, c;
	unsigned int kno, *pno;
	bool deleted = false;

	if (!ws->root) {
		printf("** Empty.\n");
		return false;
	}

	i = 0;
	words[0] = '\0';
	while ((c = getchar()) != (int)'\n'  && i < SSIZE-2)
		words[i++] = c;

	if (i > 0) {
		if (words[i-1] == '.') words[i-1] = '\0'; //맨마지막 마침표 제거
		else words[i] = '\0';
	}
	pstr = words;
	while (um_whites (*pstr)) pstr++;  //앞부분 whitespace 제거

	while (*pstr) {  //문장 길이(끝) 만큼 단어 단위로 반복
		i = 0;
		while (!is_whitespace (*pstr) && *pstr && i < ASIZE-2)
			aword[i++] = *pstr++;
		aword[i] = '\0';

		//삭제된 단어의 고유번호를 리턴 받아서 큐에 저장(재사용 하기 위해)
		kno = tw1_drop_word_run (ws, wi, aword, &deleted);
		if (deleted) {
			pno = malloc (sizeof(unsigned int));
			if (pno) {
				*pno = kno;
				que_enqueue (queue, pno);	//삭제된 고유번호를 큐에 저장
			} else return false;	//메모리 할당 실패
		}
		while (um_whites (*pstr) ) pstr++;  //whitespace 제거
	}
	return deleted;
}

//리프노드에 있는 키들(숫자_조합)을 검색하여 출력
void tw1_display (BTREE* hb[], BTREE* pi_src, BTREE* pi_trg)
{
	register int i, h;
	unsigned int cnt=0, sum=0;
	NODE5*		 node;

	for (h = 1; h < HASHSIZE; h++)
	{
		if (! hb[h]->root) continue;

		node = hb[h]->root;
		sum += hb[h]->kcnt;

		//첫번째 리프 노드로 이동
		while (!node->is_leaf)
			node = (NODE5*)node->pointers[0];	//트리 높이만큼 반복

		while (true) {
			for (i = 0; i < node->num_keys; i++) {
				printf ("%u:", ++cnt);
				//pt->outkey (node->keys[i]);
				_tw1_trans_key_data (pi_src, node->keys[i], 0);  //숫자조합 검색
				printf (" >>");
				//pt->outdata (node->pointers[i]);
				_tw1_trans_key_data (pi_trg, node->pointers[i], 0);  //숫자조합 번역(검색)

				if (!(cnt % (node->num_keys*2))) {
					printf ("\n* %u/%u(%d/%d) ", cnt, sum, h, HASHSIZE-1);
					if (tw1_qn_answer ("Would you like to see more? [Y/n] ", FLAG_YES) == FLAG_NO) {
						h = HASHSIZE;
						break;
					}
				} //if
			} //for
			if (h == HASHSIZE) break; //while exit

			if (node->pointers[hb[h]->order - 1])
				node = (NODE5*)node->pointers[hb[h]->order - 1];	//다음 리프 노드
			else break;
		} //while
	} //for

	if (cnt==0) printf("** Empty.\n");
	printf ("\n");
}

//단어 보정(교정단어 교정)
char* tw1_revision (BTREE* rs, char* skey)
{
	void *data;
	data = bpt_search (rs, skey);
	if (data) {
		str_copy (skey, data);
		return skey;
	}
	return NULL;
}

//단어를 words 배열에 입력 받음
int _tw1_words_getchar (char* prompt, char words[], BTREE* ws)
{
	int	c;
	register int i, j, length=0;
	char akey[ASIZE], *pkey, *data;

	words[0] = '\0';
	printf (prompt);

	//키보드에서 문자입력 받음
	//while ((c = getchar()) != (int)'\n' && length < SSIZE-2) words[length++] = c;
	do {
		c = getchar();		//엔터가 입력될때 까지 버퍼링
		if (length && c == '\t') {	//탭키로 단어 검색
			fflush (stdin);			//키보드버퍼 비움
			j = 0;
			for (i = length-1; i >= 0; i--) {
				if (um_whites (words[i])) break;
				akey[j++] = words[i];
			}
			if (j < 2) break;

			akey[j] = '\0';
			str_reverse (akey);
			//printf ("%s\n", akey);
			pkey = str_lower (akey);	//소문자로 변환
			data = bpt_search_str_unique_like (ws, pkey);
			if (data) {	//검색단어가 1개만 있을때
				data = (char*)data + str_len (pkey);
				//printf ("%s", (char*)data);		//검색된 단어의 뒤부분
				while (*(char*)data) {
                    words[length] = *(char*)data;
                    length++;
                    data++;
                }
			}
			words[length] = '\0';
			printf ("\n");
			printf (prompt);
			printf ("%s", words);

		} else words[length++] = c;

	} while (c != '\n' && length < SSIZE-2);

	//문징 길이 제한
	if (length >= SSIZE-2) {
		printf ("## This sentence is too long to insert.\n");
		return -1;
	}
	words[length] = '\0';
	if (is_space_mark (words[0])) return 0;

	//번역 문장 보관용 스택에 입력
	tw2_stack_push (words);

	return length;
}

///문자열을 입력받아 단어 단위의 인덱스 조합을 keys 배열에 저장
//flag가 FLAG_INSERT일때 단어 저장
int tw1_words_input (char* prompt, char keys[], BTREE* ws, BTREE* wi, BTREE* rs, QUEUE* que_kno, int flag)
{
	char words[SSIZE], *pwords, akey[ASIZE], *pkey, *data;
	register int i;
	unsigned int kno;
	int		cnt=0, length;
	STACK*	stack1;	//단어 보정용 스택
	char akey2[ASIZE], *mkey;

	if (flag==FLAG_AUTO || flag==FLAG_NONE)		//파일에서 입력(FLAG_AUTO:번역입력, FLAG_NONE:번역)
		str_copy (words, keys);
	else
		if (_tw1_words_getchar (prompt, words, ws) <= 0) return -1;	//단어를 words 배열에 입력 받음

	pwords = str_trim (words);		//앞뒤 whitespace 제거
	length = str_len (pwords) - 1;
	if (length < 0) return -1;

	if (*(pwords+length) == '.') *(pwords+length) = '\0'; //맨마지막 마침표 제거
	pwords = str_lower (pwords);	//소문자로 변환

	stack1 = stack_create ();		//단어 보정용 스택 생성

	i = 0;
	keys[0] = '\0';
	while (*pwords || *akey) {
		if (i == 0) {
			while (!is_space_mark (*pwords) && *pwords && i < ASIZE-2) akey[i++] = *pwords++;
			akey[i] = '\0';
		}
		//단어 길이 제한
		if (i >= ASIZE-2) {
			printf ("## A word is too long to insert.\n");
			break;
		}
		if (*akey) {
			//단어 보정
			pkey = tw1_revision (rs, akey);
			if (pkey) {	//단어 보정된 것이 있다면 스택에 저장
				while (*pkey) {
					i = 0;
					while (!is_whitespace (*pkey) && *pkey) akey2[i++] = *pkey++;
					akey2[i] = '\0';
					//스택에 입력
					mkey = malloc (str_len (akey2) + 1);
					str_copy (mkey, akey2);
					stack_push_top (stack1, mkey);

					while (um_whites (*pkey)) pkey++;  //whitespace 제거
				}
				pkey = stack_bottom (stack1, FLAG_VIEW);

			} else pkey = akey;	//보정 없음

			while (pkey) {
				if (flag == FLAG_INSERT || flag == FLAG_AUTO) {
					kno = _tw1_insert_to_btree (ws, wi, pkey, que_kno);	//단어사전에 추가
					if (kno != UIFAIL) {
						str_cat (keys, uint_to_str (kno, akey));
						str_cat (keys, "_");
						cnt++;
					} else {
						printf ("## Key index number(kno) error!!\n");
						*pwords = '\0';
						break;
					}
				} else {
					data = bpt_search (ws, pkey);  //단어사전에서 숫자키 검색
					if (data) {
						//btree1->outdata (data);  //있음
						str_cat (keys, uint_to_str (*(int*)data, akey));
						str_cat (keys, "_");
						cnt++;
					}
				}
				pkey = stack_bottom (stack1, FLAG_VIEW);
			} //while

			//메모리 할당된 스택이 있다면 노드 해제
			if (stack1->count && stack_drop_from_top (stack1, FLAG_NONE))
				printf ("## Error occured at the stack deletion.\n");
		} //if

		//문장부호 처리
		i = 0;
		while (is_smark (*pwords)) akey[i++] = *pwords++;
		akey[i] = '\0';

		while (um_whites (*pwords)) pwords++;  //whitespace 제거
	} //while

	//메모리 할당된 스택 모두 삭제
	if (stack_drop_from_top (stack1, FLAG_DELETE))
		printf ("## Error occured at the stack destroying.\n");

	return cnt;	//유효 단어 수
}

//단어를 입력받아 B+트리에 저장하고 숫자키를 반환 (random 테스트에서 사용)
int tw1_test_insert (BTREE* ws, BTREE* wi, char* words, char keys[], QUEUE* que_kno)
{
	char akey[ASIZE];
	register int i=0, cnt=0;
	unsigned int kno;

	while (um_whites (*words) ) words++;  //앞부분 whitespace 제거
	if (! *words) return 0;

	keys[0] = '\0';
	akey[0] = '\0';
	while (*words || *akey) {
		if (i == 0) {
			while (!is_space_mark (*words) && *words && i < ASIZE-2)
				akey[i++] = *words++;
			akey[i] = '\0';
		}
		if (*akey) {
			//단어 길이 제한
			if (i >= ASIZE-2) {
				printf ("## A word is too long to insert.\n");
				return -1;
			}
			kno = _tw1_insert_to_btree (ws, wi, akey, que_kno);	//단어 사전에 추가
			if (kno == UIFAIL) {
				printf ("## Key index number(kno) error!!\n");
				return -1;
			}
			//문장 길이 제한
			if (str_len (keys) > SSIZE - ASIZE) {
				printf ("## This sentence is too long to insert.\n");
				return -1;
			}
			str_cat (keys, uint_to_str (kno, akey));	//숫자조합키
			str_cat (keys, "_");
			cnt++;
		}
		//문장부호 처리
		i = 0;
		while (is_smark (*words)) akey[i++] = *words++;
		akey[i] = '\0';

		while (um_whites (*words)) words++;  //whitespace 제거
	}
	return cnt;  //숫자키 개수
}

//난수(정수와 문자열 최대 10자리)를 발생하여 삽입과 삭제를 반복실행(loop_max: 반복회수)
//키가 70% 정도 삽입되고, 30%는 삭제됨.
//반복을 100만번 수행시 약 100M 바이트의 메모리 소비. (개당 약100바이트)

///단어 입력 테스트
void _tw1_test_ins_word_random (BTREE* ws, BTREE* wi, unsigned int cnt_loop, QUEUE* queue)
{
	char *in_word;
	unsigned int sno, ino, dno;
	double fi=0.0, fd=0.0;

	time_rand_seed_init ();
	sno = 0;		//순번
	ino = ws->kcnt;	//입력된수
	dno = 0;		//삭제된수
	while (sno++ < cnt_loop) {
		in_word = time_get_random_str (1);
		//단어사전에 추가
		if (_tw1_insert_to_btree (ws, wi, in_word, queue) == UIFAIL) break;
		printf ("%c** Inserting... SK:%10u, IK:%10u, SC:%10u, IC:%10u", CR, ws->kno, wi->kno, ws->kcnt, wi->kcnt);
	} //while 2의32승 = 4294967296 = 약42억

	ino = ws->kcnt - ino;
	if (--sno > 0) {
		fi = (double)ino / (double)sno * 100;  //삽입율
		fd = (double)dno / (double)sno * 100;  //삭제율
	}
	printf ("\n** Completed(%u), Inserted(%u: %.2f), Deleted(%u: %.2f)\n\n", sno, ino, fi, dno, fd);
}

///단어 삭제 테스트
void _tw1_test_del_word_random (BTREE* ws, BTREE* wi, unsigned int cnt_loop, QUEUE* queue)
{
	char *in_word;
	unsigned int sno, ino, dno, kno;
	unsigned int *pno;
	double fi=0.0, fd=0.0;
	bool deleted = false;

	time_rand_seed_init ();
	sno = 0;		//순번
	ino = 0;		//입력된수
	dno = 0;		//삭제된수
	while (sno++ < cnt_loop) {
		in_word = time_get_random_str (1);
		//단어사전에서 삭제
		if ((kno = tw1_drop_word_run (ws, wi, in_word, &deleted)) == UIFAIL) break;
		if (deleted) {
			//printf ("deleted: %s, %u\n", in_word, kno);
			dno++;
			pno = malloc (sizeof(unsigned int));
			if (pno) {
				*pno = kno;
				//삭제된 고유번호를 큐에 저장
				que_enqueue (queue, pno);
			} else return;
		}
		printf ("%c** Deleting... SN:%10u, IN:%10u, DN:%10u, KW:%s", CR, sno, ino, dno, in_word);
	} //while 2의32승 = 4294967296 = 약42억

	printf ("\n** Deleted. SK:%10u, IK:%10u, SC:%10u, IC:%10u\n", ws->kno, wi->kno, ws->kcnt, wi->kcnt);
	if (--sno > 0) {
		fi = (double)ino / (double)sno * 100;  //삽입율
		fd = (double)dno / (double)sno * 100;  //삭제율
	}
	printf ("** Completed(%u), Inserted(%u: %.2f), Deleted(%u: %.2f)\n\n", sno, ino, fi, dno, fd);
}

//번역 랜덤 테스트
void tw1_test_random (BTREE* ws[], BTREE* wi[], BTREE** hb[], unsigned int cnt_loop, QUEUE* qk[])
{
	char *in_word;
	unsigned int ucnt, h;
	int  mode = 0, k;
	char keys[2][SSIZE];
	NODE5 *leaf;
	//void *data;
	BTREE *t1, *t2;

	//B+트리 A,B에 무작위 삽입과 삭제 (단어 사전 삽입, 삭제 테스트)
	//단어키가 삭제되면 기존의 번역키 데이터가 달라짐
	{
		//단어사전 A
		_tw1_test_ins_word_random (ws[0], wi[0], cnt_loop, qk[0]);	//입력 테스트
		///_tw1_test_del_word_random (ws[0], wi[0], cnt_loop, qk[0]);	//삭제 테스트
		//단어사전 B
		///_tw1_test_ins_word_random (ws[1], wi[1], cnt_loop, qk[1]);	//입력 테스트
		///_tw1_test_del_word_random (ws[1], wi[1], cnt_loop, qk[1]);	//삭제 테스트
	}

	//B+트리 A,B에 입력 (번역키 입력 및 삭제 테스트)
	printf ("\n");
	ucnt = 0;
	while (ucnt++ < cnt_loop)
	{
		in_word = time_get_random_str (ucnt % 10 + 1);
		if (tw1_test_insert (ws[mode], wi[mode], in_word, keys[mode], qk[mode]) > 0) {
			in_word = time_get_random_str (ucnt % 10 + 1);
			if (tw1_test_insert (ws[!mode], wi[!mode], in_word, keys[!mode], qk[!mode]) > 0) {
				//번역용 B+트리에 입력
				printf ("** Trans Key Inserting: %s: %s\n", keys[mode], keys[!mode]);
				h = hash_value (keys[mode]);
				t1 = hb[mode][h];
				h = hash_value (keys[!mode]);
				t2 = hb[!mode][h];
				_tw1_insert_to_btree_trans (t1, t2, keys[mode], keys[!mode]);

				if (!(ucnt%5) && (k = bpt_find_leaf_key (t1, keys[mode], &leaf)) >= 0) {
					printf ("** Trans Key Deleting: %s\n", keys[mode]);
					//번역용 B+트리에서 삭제
					_tw1_trans_key_finder (leaf, k, keys[mode], t1, hb[!mode], wi[!mode], FLAG_NONE);
				}
			}
		}
	} //while

	/*
	//번역 테스트
	printf ("\n");
	ucnt = 0;
	while (ucnt++ < cnt_loop)
	{
		keys[mode][0] = '\0';
		in_word = time_get_random_str (ucnt % 10 + 1);
		printf ("\n** Translating: %s\n", in_word);
		data = bpt_search (ws[mode], in_word);  //단어사전에서 숫자키 검색
		if (data) {
			//btree1->outdata (data);  //있음
			str_cat (keys[mode], uint_to_str (*(int*)data, keys[!mode]));
			str_cat (keys[mode], "_");
		}
		_tw1_trans_search (keys[mode], hb, wi, mode, 1);
	}
	*/
}

char* _tw1_prompt (int mode)
{
	static char *prompt[] = {"English>> ", "한글>> "};
	return prompt[mode];
}

//관리자 메뉴
char* _tw1_manager_menu (int mode)
{
	printf ("\n");
	printf ("%s for MANAGER\n", TWVersion);

	if (mode) {
		printf ("A: 단어색인(i) 보기\n");
		printf ("B: 단어색인(s) 보기\n");
		printf ("C: 번역키 색인 보기\n");
		printf ("\n");
		printf ("D: 교정단어 입력\n");
		printf ("E: 교정단어 삭제\n");
		printf ("F: 교정단어 입력(파일)\n");
		printf ("G: 교정단어 출력(파일)\n");
		printf ("\n");
		printf ("H: 유료회원 개인키 출력\n");
		printf ("I: 유료회원 개인키 입력\n");
		printf ("\n");
		printf ("J: 캡션문장 입력(파일)\n");
		printf ("K: 캡션문장 출력(파일)\n");
		printf ("L: 캡션문장 모두 삭제\n");

		printf ("x: 종료\n");

		printf ("\n");
		printf ("~: Delete Word\n");
		printf ("!: Delete All TransKey\n");
		printf ("@: Delete All Dic Words\n");
		printf ("#: Delete All Revision\n");

	} else {
		printf ("A: View Words Index(i)\n");
		printf ("B: View Words Index(s)\n");
		printf ("C: View TransKey Index\n");
		printf ("\n");
		printf ("D: Insert Revision Words\n");
		printf ("E: Delete Revision Words\n");
		printf ("F: Import Revision Words(File)\n");
		printf ("G: Export Revision Words(File)\n");
		printf ("\n");
		printf ("H: Member Private Key Output\n");
		printf ("I: Member Private Key Input\n");
		printf ("\n");
		printf ("J: Import Caption(File)\n");
		printf ("K: Export Caption(File)\n");
		printf ("L: Delete All Caption\n");

		printf ("x: Exit\n");

		printf ("\n");
		printf ("~: Delete Word\n");
		printf ("!: Delete All TransKey\n");
		printf ("@: Delete All Dic Words\n");
		printf ("#: Delete All Revision\n");
	}
	printf ("\n");

	return _tw1_prompt (mode);
}

//관리자 메뉴 실행
int tw1_manager (BTREE* ws[], BTREE* wi[], BTREE** hb[], int mode, BTREE* rs, QUEUE* qk[])
{
	char cmd, *prompt;
	int  isave = 0, h;

	prompt = _tw1_manager_menu (mode);
	cmd = 'h';
	while (cmd != 'x')
	{
		printf ("Command> ");
		cmd = a_lower ((char)getchar());
		if (cmd != '\n') while (getchar() != (int)'\n');

		switch (cmd) {
			case 'a':  //단어사전 A 조회
				bpt_print (wi[mode]);
				bpt_print_leaves (wi[mode]);
				break;
			case 'b':  //단어사전 B 조회
				bpt_print (ws[mode]);
				bpt_print_leaves (ws[mode]);
				break;
			case 'c':  //번역키 조회
				h = tw2_getchar_hash_value ();
				if (h >= 0) {
					bpt_print (hb[mode][h]);
					bpt_print_leaves (hb[mode][h]);
				}
				break;

			case 'd':	//교정단어 입력
				if (tw2_rev_word_input (rs) > 0) {
					fio_write_to_file_trans (FNAME_DAT0, rs);
					printf ("** Saved.\n");
				}
				break;
			case 'e':	//교정단어 삭제
				if (tw2_rev_word_delete (rs) > 0) {
					fio_write_to_file_trans (FNAME_DAT0, rs);
					printf ("** Saved.\n");
				}
				break;
			case 'f':	//교정단어 입력(파일)
				if (tw2_rev_word_import (rs) > 0) {
					fio_write_to_file_trans (FNAME_DAT0, rs);
					printf ("** Saved.\n");
				}
				break;
			case 'g':	//교정단어 출력(파일)
				tw2_rev_word_export (rs);
				break;

			case 'h':	//유료회원 개인키(복호화) 출력
				tw2_member_key_output (FNAME_KEY);
				break;
			case 'i':	//유료회원 개인키(암호화) 입력(저장)
				tw2_member_key_input (FNAME_KEY, FLAG_YES);
				break;

			case 'j':  //파일에서 CAPTION 문장들 입력(mode 구분, 0:영문(한글번역), 1:한글(영문번역))
				if (tw2_import_from_file (ws, wi, hb, mode, rs, qk, FLAG_CAP) > 0) {
					fio_write_to_file_trans (FNAME_DAT0, rs);
					printf ("** Saved.\n");
					isave++;
				}
				break;
			case 'k':	//번역키에서 캡션 문장들을 파일에 출력
				tw2_export_to_file (hb, wi, mode, 0);
				break;
			case 'l':	//번역키에서 캡션 문장들 삭제 (교정단어도 같이 삭제됨)
				bpt_drop (&hb[0][0], &hb[1][0]);
				isave++;
			case '#':	//교정단어 삭제
				bpt_drop_leaves_nodes (rs, rs->root);
				rs->root = NULL;
				fio_write_to_file_trans (FNAME_DAT0, rs);
				printf ("\n");
				isave++;
				break;

			case '~':  //단어삭제 (단어사전에 오직 하나 남아 있을때, 필요없는 단어일때 사용)
				if (tw1_qn_answer ("고유번호는 큐에 보관됩니다. 작업진행 하시겠습니까? [y/N] ", FLAG_NO) == FLAG_YES) {
					printf (prompt);
					if (tw1_drop_word (ws[mode], wi[mode], qk[mode])) {
						printf ("\n** Deleted.\n");
						isave++;
					} else printf ("\n** Not exist.\n");
				}
				break;
			case '!':  //번역키 모두 삭제
				for (h=1; h < HASHSIZE; h++)
					bpt_drop (&hb[0][h], &hb[1][h]);
				isave++;
				break;
			case '@':  //단어사전 A,B 모두 삭제
				bpt_drop (&ws[0], &wi[0]);
				bpt_drop (&ws[1], &wi[1]);
				isave++;
				break;

			case 'x': break;	//종료
			default:
				prompt = _tw1_manager_menu (mode);

		} //switch

	} //while

	printf("\n");
	return isave;
}

//회원 메뉴 (유료)
char* _tw1_member_menu (int mode)
{
	printf ("\n");
	printf ("%s for MEMBER\n", TWVersion);

	if (mode) {
		printf ("F: 파일 번역\n");
		printf ("I: 번역정보 입력(파일)\n");
		printf ("E: 번역정보 출력(파일)\n");
		printf ("U: 단어 수정\n");
		//printf ("D: 번역정보 모두 삭제\n");
		printf ("H: 도움말\n");
		printf ("x: 종료\n");
	} else {
		printf ("F: File Translation\n");
		printf ("I: Import (File)\n");
		printf ("E: Export (File)\n");
		printf ("U: Update Word\n");
		//printf ("D: Delete All\n");
		printf ("H: Help\n");
		printf ("x: Exit\n");
	}
	printf ("\n");

	return _tw1_prompt (mode);
}

//회원 메뉴(유료) 실행
int tw1_member (BTREE* ws[], BTREE* wi[], BTREE** hb[], int mode, BTREE* rs, QUEUE* qk[])
{
	char cmd, *prompt;
	int  isave = 0;
	static bool bManager = false;	//관리자 로그인 여부

	prompt = _tw1_member_menu (mode);
	cmd = 'h';
	while (cmd != 'x')
	{
		printf ("Command> ");
		cmd = a_lower ((char)getchar());
		if (cmd != '\n') while (getchar() != (int)'\n');

		StackTW_Enable = false;

		switch (cmd) {
			case 'f':	//파일안의 문장들 번역(영한-한영 모두 번역)
				StackTW_Enable = true;
				tw2_file_translation (ws, wi, hb, mode, rs, qk, FLAG_AUTO);	//회원(제한없이 번역)
				break;
			case 'i':  //파일에서 번역된 문장들 입력(mode 구분, 0:영문(한글번역), 1:한글(영문번역))
				isave += tw2_import_from_file (ws, wi, hb, mode, rs, qk, FLAG_INSERT);
				break;
			case 'e':	//번역키에서 문장(단어)들을 파일에 출력
				tw2_export_to_file (hb, wi, mode, -1);
				break;
			case 'u':  //단어 수정 (고유번호 그대로 유지)
				isave += tw2_word_update (ws[mode], wi[mode], prompt);
				break;
			/*
			case 'd':  //번역정보 모두 삭제 (캡션 문장은 제외)
				if (tw1_qn_answer ("번역정보들을 모두 비웁니다. 작업진행 하시겠습니까? [y/N] ", FLAG_NO) == FLAG_YES) {
					for (h=1; h < HASHSIZE; h++)
						bpt_drop (&hb[0][h], &hb[1][h]);	//번역키 모두 삭제
					//단어사전 A,B 모두 삭제
					bpt_drop (&ws[0], &wi[0]);
					bpt_drop (&ws[1], &wi[1]);
					isave++;
				}
				break;
			*/

			case 'h':  //도움말
				if (! fio_read_help (FNAME_DAT4))
					prompt = _tw1_member_menu (mode);
				//printf ("\n");
				break;

			case 'x': break;	//종료

			case '@':	//관리자 메뉴로 이동
				if (bManager || tw2_getchar_manager_pw ()) {
					isave += tw1_manager (ws, wi, hb, mode, rs, qk);
					bManager = true;
				}
			default:
				prompt = _tw1_member_menu (mode);

		} //switch

	} //while

	printf("\n");
	return isave;
}

void tw1_save (BTREE* ai, BTREE* bi, BTREE* hbta[], BTREE* rs)
{
	register int i;
	char afname[ASIZE], anum[ASIZE];

	//교정단어 및 문장캡션 저장
	//fio_write_to_file_trans (FNAME_DAT0, rs);

	//단어사전(숫자키) 파일에 저장
	fio_write_to_file (FNAME_DIC0, ai);
	fio_write_to_file (FNAME_DIC1, bi);

	//번역키 파일에 저장
	for (i = 0; i < HASHSIZE; i++) {
		afname[0] = '\0';
		str_cat (afname, FNAME_IDX);	//"twd"
		str_cat (afname, uint_to_str (i, anum));
		str_cat (afname, FNAME_EXT);	//".twi"
		fio_write_to_file_trans (afname, hbta[i]);
	}
}

//통계 정보 출력
void tw1_statis (BTREE* ws[], BTREE* wi[], BTREE** hb[], BTREE* rs, QUEUE** qk)
{
	register int i;
	unsigned int sum0=0, sum1=0;	//합계이므로 데이터 타입이 커야함.
	unsigned int min_a, min_b, max_a, max_b;

	printf ("%s\n\n", TWVersion);

	printf ("* Dictionay Tree Order  :%6d\n", ws[0]->order);	//B_ORDER
	printf ("* Translation Hash Size :%6d\n", HASHSIZE);		//HASHSIZE
	printf ("* Revision Index Count  :%6d\n", rs->kcnt);		//단어교정키 개수
	printf ("* Words A Queue Count   :%6d\n", qk[0]->count);	//단어사전 A 삭제된 고유번호 개수
	printf ("* Words B Queue Count   :%6d\n", qk[1]->count);	//단어사전 A 삭제된 고유번호 개수

	printf ("\n");
	printf ("* Words A Index(i) Count            :%10u\n", wi[0]->kcnt);
	if (wi[0]->kcnt != ws[0]->kcnt)	//항상 같아야 함(중복키 없음)
	printf ("# Words A Index(s) Count            :%10u\n", ws[0]->kcnt);

	printf ("* Words A Index(i) Serial           :%10u\n", wi[0]->kno);
	if (wi[0]->kno != ws[0]->kno)	//항상 같아야 함(중복키 없음)
	printf ("# Words A Index(s) Serial           :%10u\n", ws[0]->kno);

	printf ("* Words B Index(i) Count            :%10u\n", wi[1]->kcnt);
	if (wi[1]->kcnt != ws[1]->kcnt)	//항상 같아야 함(중복키 없음)
	printf ("# Words B Index(s) Count            :%10u\n", ws[1]->kcnt);

	printf ("* Words B Index(i) Serial           :%10u\n", wi[1]->kno);
	if (wi[1]->kno != ws[1]->kno)	//항상 같아야 함(중복키 없음)
	printf ("# Words B Index(s) Serial           :%10u\n", ws[1]->kno);

	printf ("\n");
	printf ("* Translation[0] Index(A) Count     :%10u\n", hb[0][0]->kcnt);
	printf ("* Translation[0] Index(B) Count     :%10u\n", hb[1][0]->kcnt);
	printf ("\n");
	min_a = hb[0][1]->kcnt;
	min_b = hb[1][1]->kcnt;
	max_a = hb[0][1]->kcnt;
	max_b = hb[1][1]->kcnt;
	for (i = 1; i < HASHSIZE; i++) {
		min_a = um_min(min_a, hb[0][i]->kcnt);	//최소
		min_b = um_min(min_b, hb[1][i]->kcnt);
		max_a = um_max(max_a, hb[0][i]->kcnt);	//최대
		max_b = um_max(max_b, hb[1][i]->kcnt);
		//번역키 개수 합계
		sum0 += hb[0][i]->kcnt;
		sum1 += hb[1][i]->kcnt;
	}
	printf ("* Translation[#] Index(A) Min Count :%10u\n", min_a);
	printf ("* Translation[#] Index(B) Min Count :%10u\n", min_b);
	printf ("* Translation[#] Index(A) Max Count :%10u\n", max_a);
	printf ("* Translation[#] Index(B) Max Count :%10u\n", max_b);
	printf ("\n");
	printf ("* Translation[#] Index(A) All Sum   :%10u\n", sum0);
	if (sum0 != sum1)	//합계는 항상 같아야 함
		printf ("# Translation[#] Index(B) All Sum   :%10u\n", sum1);

	printf ("\n");

	#ifdef __LINUX
		mem_info ();	//메모리 사용 정보
	#endif
}

char* _tw1_menu (int mode, int isave)
{
	printf ("\n");
	printf ("%s for natural language translation\n", TWVersion);
	printf ("  written by Jung,JaeJoon(rgbi3307@nate.com) on the www.kernel.bz.\n");
	if (mode) {
		printf ("**메일이나 전화(010-2520-3307)로 이 프로그램에 대하여 문의 하세요.\n\n");

		printf ("*MENU for 한글-영어\n");
		printf ("T: 번역하기\n");
		printf ("I: 입력하기\n");
		printf ("D: 삭제하기\n");
		printf ("V: 조회하기\n");
		printf ("F: 파일번역\n");
		printf ("R: 번역복습\n");
		printf ("C: 번역전환\n");

		if (isave) printf ("S: 저장하기\n");
		printf ("A: 통계\n");
		printf ("M: 회원메뉴\n");
		printf ("H: 도움말\n");
		printf ("Q: 종료\n");

	} else {
		printf ("**Please email or call me(010-2520-3307) to ask about Algorithms.\n\n");

		printf ("*MENU for English-Korean\n");
		printf ("T: Translation\n");
		printf ("I: Insertion\n");
		printf ("D: Deletion\n");
		printf ("V: View\n");
		printf ("F: File Trans\n");
		printf ("R: Review\n");
		printf ("C: Change\n");

		if (isave) printf ("S: Save\n");
		printf ("A: Statistic\n");
		printf ("M: Member\n");
		printf ("H: Help\n");
		printf ("Q: Quit\n");
	}
	printf ("\n");

	return _tw1_prompt (mode);
}

//isave;  //저장건수
void tw1_menu_run (BTREE* ws[], BTREE* wi[], BTREE** hb[], BTREE* rs, QUEUE* qk[], int isave)
{
	char cmd, *prompt;
	int  mode, idx;
	static bool bMember = false;	//유료회원 로그인 여부

	mode = 0;   //메뉴모드(0:English, 1:Korean)
	prompt = _tw1_menu (mode, isave);

	cmd = 'h';
	while (cmd != '!')
	{
		printf ("Command> ");
		cmd = a_lower ((char)getchar());
		if (cmd != '\n') while (getchar() != (int)'\n');

		StackTW_Enable = false;

		switch (cmd) {
			case 't':  //문장 번역 --> 지속 작업
				StackTW_Enable = true;
				tw1_translation (ws, hb, wi, mode, prompt, rs, qk);
				prompt = _tw1_menu (mode, isave);
				break;
			case 'i':  //문장 입력(단어 사전에도 입력됨) --> 지속 작업
				isave += tw1_insertion (ws, wi, hb, mode, rs, qk);
				prompt = _tw1_menu (mode, isave);
				break;
			case 'd':  //문장 삭제
				isave += tw1_deletion (ws, hb, wi, mode, prompt, rs, qk);
				break;
			case 'v':  //문장 보기
				tw1_display (hb[mode], wi[mode], wi[!mode]);
				break;
			case 'f':	//파일안의 문장들 번역(영한-한영 모두 번역)
				StackTW_Enable = true;
				tw2_file_translation (ws, wi, hb, mode, rs, qk, FLAG_NONE);	//비회원(문장번역개수 제한)
				break;
			case 'r':	//Review: 번역복습
				tw2_stack_review ();	//StackTW 사용(전역스택)
				break;

			case 'c':  //번역전환
				prompt = _tw1_menu (mode = !mode, isave);
				break;
			case 's':  //저장
				tw1_save (wi[0], wi[1], hb[0], rs);
				isave = 0;
				break;
			case 'a':  //통계 출력
				tw1_statis (ws, wi, hb, rs, qk);
				break;

			case 'h':  //도움말
				if (! fio_read_help (FNAME_DAT3))
					prompt = _tw1_menu (mode, isave);
				//printf ("\n");
				break;
			case 'q':	//종료
				if (isave) tw1_save (wi[0], wi[1], hb[0], rs);	//저장 건수가 있으면, 종료전에 저장
				cmd = '!';
			case '!': break;

			case 'm':  //회원메뉴(유료)
				idx = 1;
				if (bMember || (idx = tw2_getchar_member_pw (FNAME_KEY)) >= 0) {
					if (idx > 0) {
						isave += tw1_member (ws, wi, hb, mode, rs, qk);
						bMember = true;
					} else {
						printf ("** 개인키 암호가 틀립니다.\n");
						break;
					}
				} else fio_read_help (FNAME_DAT4);

			default:
				prompt = _tw1_menu (mode, isave);

		} //switch

	} //while

	printf("\n");
}
