//
//  Source: fileio.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-12-21 파일 입출력 함수들을 작성하다.
//		2011-01-07 파일 입출력 함수들을 수정하여 리눅스와 윈도우에서 호환되도록 하다.
//		2011-01-08 파일저장 및 읽기는 키순서대로 진행되므로 B+Tree에 입력하는 속도를 개선하다.
//		2011-02-11 삭제된 단어의 고유번호 큐를 파일 입출력하는 함수를 추가하다.
//		2011-02-22 데이터 파일 import/export 함수 추가하다.
//		2011-02-22 파일안의 문장 번역기능을 추가하다.(fio_translation: 파일입력 --> 번역 --> 파일출력)
//		2011-02-25 파일 헤더에 버젼 문자열(TWVersion)을 추가하여 데이터와 프로그램의 버젼을 체크한다.
//		2011-03-01 유료회원 사용자 개인키를 암호화/복호화하여 파일 입출력하는 기능 추가하다.
//		yyyy-mm-dd ...
//

#include <stdio.h>
#include <stdlib.h>

#include "dtype.h"
#include "umac.h"
#include "ustr.h"
#include "bpt3.h"
#include "queue.h"
#include "tw1.h"
#include "tw2.h"
#include "utime.h"

#ifdef __LINUX
	#include <alloca.h>		//alloca() 함수
	#include <sys/stat.h>	//file, directory 관련 함수
	#include <sys/types.h>
#else
	#include <malloc.h>		//alloca() 함수
	#include <direct.h>		//file, directory 관련 함수
#endif


//Win32에서, warning C4996: deprecated로 선언... 메세지 제거를 위해
//프로젝트속성/구성속성/C,C++/전처리기에 _CRT_SECURE_NO_DEPRECATE 추가


//파일 헤더 읽기
int _fio_read_header (FILE *fp, char *fname, char *buf)
{
	char *sbuf = buf;

	//리눅스 fgets 함수에서 개행(LF,CR) 읽는 방식이 달라서 아래 코드로 대체
	//데이터 파일명 읽기
	while ( (*buf = fgetc (fp)) != CR) buf++;
	*buf = '\0';
	if (str_cmp (sbuf, fname)) return -1;	//파일명이 다른경우

	//버전 읽기
	buf = sbuf;
	while ( (*buf = fgetc (fp)) != CR) buf++;
	*buf = '\0';

	//fgetc (fp);	//EOF(-1)

	//HASHSIZE 읽기(번역키 데이터 호환 점검)
	buf = sbuf;
	while ( (*buf = fgetc (fp)) != CR) buf++;
	*buf = '\0';
	if (str_cmp (sbuf, HashSizeStr)) {	//HASHSIZE가 다른 경우
		printf ("Hash Size (%s != %s), ", sbuf, HashSizeStr);
		return 0;
	}
	return 1;
}

//파일 헤더 쓰기
void _fio_write_header (FILE *fp, char *fname)
{
	char* sbuf;

	//리눅스 fputs 함수에서 개행(LF,CR) 쓰는 방식이 달라서 아래 코드로 대체
	//파일명 쓰기
	while (*fname) fputc (*fname++, fp);
	fputc (CR, fp);		//CR

	//버젼 쓰기
	sbuf = TWVersion;
	while (*sbuf) fputc (*sbuf++, fp);
	fputc (CR, fp);		//CR

	//fputc (EOF, fp);	//EOF(-1)

	//HASHSIZE 쓰기(번역키 데이터 호환)
	sbuf = HashSizeStr;
	while (*sbuf) fputc (*sbuf++, fp);
	fputc (CR, fp);		//CR
}

//파일에서 사전정보를 읽어서 B+트리에 할당 (숫자키)
unsigned int fio_read_from_file (char *fname, BTREE* btree)
{
	FILE	*fp;
	NODE5	*leaf, *leaf_left;
	register int i, j=0;
	int		 ch, ihalf, imod;
	unsigned int sno=0;
	unsigned int *pno, kno=0, *pnoa[ASIZE];	//키포인터 배열은 B_ORDER에 의해 결정되나, 단어 길이로 넉넉하게...
	char	in_key[ASIZE], in_word[ASIZE];	//단어길이
	char	*pword, *pworda[ASIZE];
	char	prompt[] = {'+', '-', '*'};

	printf ("Reading data from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Not exist!\n");
		return 0;
	}	
	if (_fio_read_header (fp, fname, in_word) < 0) {
		printf (", Header error!\n");
		return 0;
	}
	//리프노드 엔트리(배열)의 split 위치 인덱스
	//B_ORDER가 짝수일때, 리프분할되는 개수가 일정해짐
	ihalf = _bpt_half_order (btree->order); 

	while ((ch = fgetc (fp)) != EOF) {  //-1
		i = 0;
		while (ch != '\0' && i < ASIZE-2) {
			in_key[i++] = ch;
			ch = fgetc (fp);
		}
		in_key[i] = '\0';

		ch = fgetc (fp);
		i = 0;
		while (ch != '\0' && i < ASIZE-2) {
			in_word[i++] = ch;
			ch = fgetc (fp);
		}
		in_word[i] = '\0';

		//문자열을 unsigned int로 변환
		kno = str_to_uint (in_key);
		pno = malloc (sizeof(unsigned int));
		if (!pno) {
			printf ("## Failure to allocate pno in fio_read_from_file().\n");
			break;  //메모리 할당 실패
		}
		*pno = kno; //키고유번호

		pword = malloc (str_len (in_word) + 1);
		if (!pword) {
			printf ("## Failure to allocate pword in fio_read_from_file().\n");
			break;  //메모리 할당 실패
		}
		str_copy (pword, in_word);
		//printf ("%u:%s ", *pno, pword);		
		
		//숫자키 순서대로 진행되기 때문에 입력속도 개선했지만, 노드의 50%만 채워지는 문제 있음
		//leaf 노드가 오른쪽으로 연결된 순서대로 삽입되므로 다시 검색할 필요 없음(속도 개선)
		//리프노드 찾을 필요 없음
		//leaf = bpt_find_leaf (btree, pno, btree->compare);
		//btree->root = bpt_insert (btree, leaf, pno, pword);
		//btree->root = bpt_insert_asc (btree, &leaf, pno, pword);

		//리프 노드의 엔트리를 100% 채우기 위해서 아래 코드로 수정 (단, B_ORDER가 짝수여야 함)
		imod = (int)(sno % (btree->order-1));
		if (imod < ihalf) {	//배열의 왼쪽
			btree->root = bpt_insert_asc (btree, &leaf, pno, pword);
			if (sno == 0) leaf_left = btree->root;

			printf ("%c(%c)%u ", CR, prompt[sno%3], sno);  //읽기표시 프롬프트

		} else {	//엔터리 오른쪽 보관
			pnoa[j] = pno;
			pworda[j++] = pword;
		}
		if (imod == (ihalf-1) && j > 0) {	//보관된 엔터리를 왼쪽 리프에 채움 (리프 노드가 100% 채워짐)
			for (i = 0; i < j; i++)
				btree->root = bpt_insert_asc (btree, &leaf_left, pnoa[i], pworda[i]);
			leaf_left = leaf_left->pointers [btree->order - 1];
			j = 0;
		}
		sno++;	//순번		
	} //while

	//보관된 엔터리가 있다면 맨오른쪽 리프에 삽입
	for (i = 0; i < j; i++) {
		btree->root = bpt_insert (btree, leaf, pnoa[i], pworda[i], FLAG_INSERT);
		if (leaf->pointers [btree->order -1]) leaf = leaf->pointers [btree->order -1];	//리프분할이 다시 발생 했다면 
	}

	fclose (fp);

	printf ("%c(%c)%u ", CR, prompt[0], sno);  //읽기표시 프롬프트
	printf ("have read data: count (%u)\n", btree->kcnt);

	return sno;  //읽은 키 개수
}

//파일에서 번역키를 순서대로 읽어서 순서대로 B+트리에 할당 (노드의 엔트리 100% 채워짐)
unsigned int fio_read_trans_asc (char *fname, BTREE* btree, int flag)
{
	FILE	*fp;
	NODE5	*leaf, *leaf_left;
	register int i, j=0;
	int		 ch, ihalf, imod;
	unsigned int sno = 0;
	char	in_key[SSIZE], in_word[SSIZE];	//숫자조합 인덱스 길이
	char	*pkey, *pkeya[ASIZE], *pword, *pworda[ASIZE];
	char	prompt[] = {'+', '-', '*'};

	printf ("Reading data from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Not exist!\n");
		return 0;
	}	
	if ((ch = _fio_read_header (fp, fname, in_word)) <= 0) {
		if (ch < 0) {
			printf (", Header error!\n");
			return 0;
		}
		if (flag) return 0;	//ch==0: HASHSIZE 다름
	}
	//리프노드 엔트리(배열)의 split 위치 인덱스
	//B_ORDER가 짝수일때, 리프분할되는 개수가 일정해짐
	ihalf = _bpt_half_order (btree->order); 

	while ((ch = fgetc (fp)) != EOF) { //-1, 0x1A, ^Z
		i = 0;
		while (ch != '\0' && i < SSIZE-2) {
			in_key[i++] = ch;
			ch = fgetc (fp);
		}
		in_key[i] = '\0';

		ch = fgetc (fp);
		i = 0;
		while (ch != '\0' && i < SSIZE-2) {
			in_word[i++] = ch;
			ch = fgetc (fp);
		}
		in_word[i] = '\0';

		pkey = malloc (str_len (in_key) + 1);
		if (!pkey) {
			printf ("## Failure to allocate pkey in fio_read_trans_asc().\n");
			break;  //메모리 할당 실패
		}
		str_copy (pkey, in_key);

		pword = malloc (str_len (in_word) + 1);
		if (!pword) {
			printf ("## Failure to allocate pword in fio_read_trans_asc().\n");
			break;  //메모리 할당 실패
		}
		str_copy (pword, in_word);

		//번역키 순서대로 진행되기 때문에 입력속도 개선했지만, 노드의 50%만 채워지는 문제 있음
		//leaf 노드가 오른쪽으로 연결된 순서대로 삽입되므로 다시 검색할 필요 없음(속도 개선)
		//리프노드 찾을 필요 없음
		//leaf = bpt_find_leaf (btree, pkey, btree->compare);
		//btree->root = bpt_insert (btree, leaf, pkey, pword);
		//btree->root = bpt_insert_asc (btree, &leaf, pkey, pword);

		//리프 노드의 엔트리를 100% 채우기 위해서 아래 코드로 수정 (단, B_ORDER가 짝수여야 함)
		imod = (int)(sno % (btree->order-1));
		if (imod < ihalf) {	//배열의 왼쪽
			btree->root = bpt_insert_asc (btree, &leaf, pkey, pword);
			if (sno == 0) leaf_left = btree->root;
			
			printf ("%c(%c)%u ", CR, prompt[sno%3], sno);  //읽기표시 프롬프트

		} else {	//엔터리 오른쪽 보관
			pkeya[j] = pkey;
			pworda[j++] = pword;
		}
		if (imod == (ihalf-1) && j > 0) {	//보관된 엔터리를 왼쪽 리프에 채움 (리프 노드가 100% 채워짐)
			for (i = 0; i < j; i++)
				btree->root = bpt_insert_asc (btree, &leaf_left, pkeya[i], pworda[i]);
			leaf_left = leaf_left->pointers [btree->order - 1];
			j = 0;
		}
		sno++;	//순번
	} //while

	//보관된 엔터리가 있다면 맨오른쪽 리프에 삽입
	for (i = 0; i < j; i++) {
		btree->root = bpt_insert (btree, leaf, pkeya[i], pworda[i], FLAG_INSERT);
		if (leaf->pointers [btree->order -1]) leaf = leaf->pointers [btree->order -1];	//리프분할이 다시 발생 했다면 
	}

	fclose (fp);

	printf ("%c(%c)%u ", CR, prompt[0], sno);  //읽기표시 프롬프트
	printf ("have read data: count (%u)\n", btree->kcnt);

	return sno;  //읽은 키 개수
}

//HASHSIZE가 변경된 경우 해시값별로 B+트리에 저장(노드의 엔트리 100% 채워지지 않음)
unsigned int fio_read_trans_hash (char *fname, BTREE** hb[], int sh)
{
	FILE	*fp;
	NODE5	*leaf;
	register int i;
	int		 ch;
	unsigned int sno=0, h;
	char	in_key[SSIZE], in_word[SSIZE];	//숫자조합 인덱스 길이
	char	*pkey, *pword;
	char	prompt[] = {'+', '-', '*'};

	printf ("Reading data(hash) from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Not exist!\n");
		return 0;
	}	
	if (_fio_read_header (fp, fname, in_word) < 0) {
		printf (", Header error!\n");
		return 0;
	}
	while ((ch = fgetc (fp)) != EOF) { //-1
		i = 0;
		while (ch != '\0' && i < SSIZE-2) {
			in_key[i++] = ch;
			ch = fgetc (fp);
		}
		in_key[i] = '\0';

		ch = fgetc (fp);
		i = 0;
		while (ch != '\0' && i < SSIZE-2) {
			in_word[i++] = ch;
			ch = fgetc (fp);
		}
		in_word[i] = '\0';

		pkey = malloc (str_len (in_key) + 1);
		if (!pkey) {
			printf ("## Failure to allocate pkey in fio_read_trans_asc().\n");
			break;  //메모리 할당 실패
		}
		str_copy (pkey, in_key);

		pword = malloc (str_len (in_word) + 1);
		if (!pword) {
			printf ("## Failure to allocate pword in fio_read_trans_asc().\n");
			break;  //메모리 할당 실패
		}
		str_copy (pword, in_word);

		//해시값에 따라 B+트리 A 리프찾은후 삽입
		h = (sh==0) ? sh : hash_value (pkey);
		leaf = bpt_find_leaf (hb[0][h], pkey, hb[0][h]->compare);
		hb[0][h]->root = bpt_insert (hb[0][h], leaf, pkey, pword, FLAG_INSERT);

		//해시값에 따라 B+트리 B 리프찾은후 삽입
		h = (sh==0) ? sh : hash_value (pword);
		leaf = bpt_find_leaf (hb[1][h], pword, hb[1][h]->compare);
		hb[1][h]->root = bpt_insert (hb[1][h], leaf, pword, pkey, FLAG_INSERT);
		
		printf ("%c(%c)%u ", CR, prompt[sno%3], sno);  //읽기표시 프롬프트
		sno++;	//순번
	} //while

	fclose (fp);

	printf ("%c(%c)%u ", CR, prompt[0], sno);  //읽기표시 프롬프트
	printf ("have read data(hash): count (%u)\n", sno);

	return sno;  //읽은 키 개수
}

//B+트리의 사전정보를 파일에 저장
unsigned int fio_write_to_file (char *fname, BTREE* btree)
{
	register int i, j, height;
	unsigned int keys_cnt = 0;
	NODE5*	node = btree->root;
	FILE	*fp;
	char	akey[ASIZE];
	char	prompt[] = {'+', '-', '*'};

	fp = fopen (fname, "w");
	if (fp == NULL) {
		printf ("File(%s) creating error!\n", fname);
		return 0;
	}	
	_fio_write_header (fp, fname);  //파일 헤더 쓰기

	printf ("Writing data to the %s...\n", fname);
	if (!node) {
		printf (", Empty.\n");
		fclose (fp);
		return 0;
	}

	//첫번째 리프 노드 찾음
	height = 1;
	while (!node->is_leaf) {
		node = (NODE5*)node->pointers[0];
		height++;  //트리 높이
	}
	while (btree->kcnt > 0) {		
		keys_cnt += node->num_keys;  //리프에 있는 키들의 수 덧셈
		for (i = 0; i < node->num_keys; i++) {			
			//btree->outkey (node->keys[i]);
			uint_to_str (*(unsigned int*)node->keys[i], akey);  //정수키를 문자열키로 변환
			j = 0;
			while (fputc (akey[j++], fp));  //마지막에 널 저장됨
			//btree->outdata (node->pointers[i]);
			j = 0;
			while (fputc (*((char*)node->pointers[i] + j++), fp));  //마지막에 널 저장됨
		}
		printf ("%c(%c)%u ", CR, prompt[keys_cnt%3], keys_cnt);  //쓰기표시 프롬프트

		if (node->pointers[btree->order - 1])
			node = (NODE5*)node->pointers[btree->order - 1];	//다음 리프 노드
		else break;		
	} //while

	fclose (fp);

	printf ("%c(%c)%u ", CR, prompt[0], keys_cnt);  //쓰기표시 프롬프트
	printf ("have written data: height (%d), count (%u)\n", height, btree->kcnt);

	return keys_cnt;
}

//B+트리의 번역정보를 파일에 저장
unsigned int fio_write_to_file_trans (char *fname, BTREE* btree)
{
	register int i, j, height;
	unsigned int keys_cnt = 0;
	NODE5*	node = btree->root;
	FILE	*fp;
	char	prompt[] = {'+', '-', '*'};

	fp = fopen (fname, "w");
	if (fp == NULL) {
		printf ("File(%s) creating error!\n", fname);
		return 0;
	}	
	_fio_write_header (fp, fname);  //파일 헤더 쓰기

	printf ("Writing data to the %s...\n", fname);
	if (!node) {
		printf (", Empty.\n");
		fclose (fp);
		return 0;
	}

	//첫번째 리프 노드 찾음
	height = 1;
	while (!node->is_leaf) {
		node = (NODE5*)node->pointers[0];
		height++;  //트리 높이
	}
	while (btree->kcnt > 0) {		
		keys_cnt += node->num_keys;  //리프에 있는 키들의 수 덧셈
		for (i = 0; i < node->num_keys; i++) {
			j = 0;
			while (fputc (*((char*)node->keys[i] + j++), fp));  //마지막에 널 저장됨
			j = 0;
			while (fputc (*((char*)node->pointers[i] + j++), fp));  //마지막에 널 저장됨			
		}
		printf ("%c(%c)%u ", CR, prompt[keys_cnt%3], keys_cnt);  //쓰기표시 프롬프트

		if (node->pointers[btree->order - 1])
			node = (NODE5*)node->pointers[btree->order - 1];	//다음 리프 노드
		else break;		
	} //while

	fclose (fp);

	printf ("%c(%c)%u ", CR, prompt[0], keys_cnt);  //쓰기표시 프롬프트
	printf ("have written data: height (%d), count (%u)\n", height, btree->kcnt);

	return keys_cnt;
}

///삭제된 단어의 고유번호를 파일에서 읽어서 queue에 저장
unsigned int fio_read_from_file_kno (char *fname, QUEUE* queue)
{
	FILE	*fp;
	register int i;
	unsigned int kno, *pno;
	int		ch;
	char	in_key[ASIZE];

	fp = fopen (fname, "r");
	if (fp == NULL) return 0;	//printf (", Not exist!\n");
	printf ("Reading data from the %s...\n", fname);

	if (_fio_read_header (fp, fname, in_key) < 0) {
		printf (", Header error!\n");
		return 0;
	}

	while ((ch = fgetc (fp)) != EOF) {  //-1, 0x1A, ^Z
		i = 0;
		while (ch != '\0' && i < ASIZE-2) {
			in_key[i++] = ch;
			ch = fgetc (fp);
		}
		in_key[i] = '\0';

		//문자열을 unsigned int로 변환
		kno = str_to_uint (in_key);
		//큐에 입력
		pno = malloc (sizeof(unsigned int));
		if (pno) {	
			*pno = kno;
			que_enqueue (queue, pno);	//삭제된 고유번호를 큐에 저장
		} else return 0;	//메모리 할당 실패
	} //while

	fclose (fp);
	printf ("have read data: queue (%d)\n", queue->count);

	return queue->count;  //읽은 키 개수
}

///queue에 있는 삭제된 단어의 고유번호를 파일에 저장
unsigned int fio_write_to_file_kno (char *fname, QUEUE* queue)
{
	register int i;
	unsigned int *pno, cnt=0;
	FILE	*fp;
	char	akey[ASIZE];

	fp = fopen (fname, "w");
	if (fp == NULL) {
		printf ("File(%s) creating error!\n", fname);
		return 0;
	}	
	_fio_write_header (fp, fname);  //파일 헤더 쓰기

	printf ("Writing data to the %s...\n", fname);
	if (queue->count == 0) {
		////printf (", Empty.\n");
		fclose (fp);
		return 0;
	}

	while (!que_is_empty (queue))	{
		if (que_dequeue (queue, (void*)&pno)) {
			uint_to_str (*pno, akey);  //정수키를 문자열키로 변환
			i = 0;
			while (fputc (akey[i++], fp));  //마지막에 널 저장됨
			free (pno);	//큐의 데이터 메모리 해제
			cnt++;
		} else break;
	} //while

	fclose (fp);
	printf ("have written data: queue (%u)\n", cnt);

	return cnt;
}

//번역 데이터 파일(txt)에서 번역행들을 읽어서 저장(입력)
//mode 구분, 0:영문(한글번역), 1:한글(영문번역)
int fio_import (char *fname, BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk)
{
	FILE	*fp;
	register int i=0;
	char	ch, row[SSIZE], rows[2][SSIZE], *rowp;
	int		idx, rown=0, cnt=0, ie;
	bool	brun;

	printf ("Reading data from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Not exist!\n");
		return 0;
	}
	rows[0][0] = '\0';
	rows[1][0] = '\0';
	//개행문자, 문장 단위로 읽음
	ch = fgetc (fp);
	while (ch != EOF) 
	{
		ie = 0;
		while (ch != '\n' && ch != EOF && i < SSIZE-2) {	//문장끝
			row[i++] = ch;
			if (um_end(ch)) ie = 1;	//문장끝 부호(. ? !)가 있는지 여부
			ch = fgetc (fp);			
		}
		if (i >= SSIZE-2) break;

		row[i] = '\0';
		rowp = str_trim_left (row);
		if (*rowp == '/' && *(rowp+1) == '/') {	//주석
			ch = fgetc (fp);
			i = 0;
			continue;
		}
		if (ch == '\n')	row[i++] =  ' ';
		ch = fgetc (fp);
		if (ch != EOF && !ie) continue;	//문장끝이 아님으로 판단하고 계속 읽음

		row[i] = '\0';
		rowp = str_trim (row);
		
		//0:영문, 1:한글, -1:특수문자
		idx = str_is_eng_kor (rowp);
		if (idx >= 0) {
			if (*rowp == '*') str_copy (rows[idx], rowp+1);
			else str_copy (rows[idx], rowp);

			brun = (mode) ? !idx: idx;	//0:영문(한글번역), 1:한글(영문번역)
			if (brun && *rows[mode])
				cnt += tw1_insertion_from_file (ws, wi, hb, mode, rs, qk, rows, -1);	//저장 문장 수 누적
		}
		printf ("%c*line: %d/%d: ", CR, cnt, ++rown);  //문장수
		i = 0;
	}

	fclose (fp);
	printf ("%d rows have saved.\n", cnt);

	return cnt;	//저장한 문장 수
}

///번역정보를 파일에 출력
int _fio_export_data (FILE *fp, BTREE* wi, void* keys)
{
	char	*ckeys, adigit[ASIZE], sbuf[SSIZE];
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

	sbuf[0] = '\0';
	ckeys = (char*)keys;
	while (*ckeys) {
		i = 0;
		while ( (adigit[i++] = *ckeys++) != '_');
		i--;
		adigit[i] = '\0';		

		*pkey = str_to_uint (adigit);
		data = bpt_search (wi, pkey);
		if (data) {
			cnt++;
			str_cat (sbuf, data);
		}
		str_cat (sbuf, " ");
	}
	sbuf[str_len (sbuf) - 1] = '\0';	//마지막 공백 제거

	//free (pkey);  //alloca 에서 스택에 할당된 메모리는 scope를 벗어나면 자동으로 해제됨
	str_cat (sbuf, ".\n");
	fputs (sbuf, fp);	//파일에 저장

	//번역 문장 보관용 스택에 입력
	if (cnt > 0) tw2_stack_push (sbuf);

	return cnt;
}

//번역정보를 버퍼에 저장
int _fio_export_buffer (char sbuf[], BTREE* wi, void* keys)
{
	char	*ckeys, adigit[ASIZE];
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
	
	ckeys = (char*)keys;
	while (*ckeys) {
		i = 0;
		while ( (adigit[i++] = *ckeys++) != '_');
		i--;
		adigit[i] = '\0';		

		*pkey = str_to_uint (adigit);
		data = bpt_search (wi, pkey);
		if (data) {
			cnt++;
			str_cat (sbuf, data);
		} else str_cat (sbuf, "~");

		str_cat (sbuf, " ");
	}
	sbuf[str_len (sbuf) - 1] = '\0';	//마지막 공백 제거

	//free (pkey);  //alloca 에서 스택에 할당된 메모리는 scope를 벗어나면 자동으로 해제됨
	return cnt;
}

///번역 데이터을 파일에 출력(저장)
void fio_export (char* fname, BTREE* pt, BTREE* pi_src, BTREE* pi_trg) 
{
	FILE*	fp;
	register int i;
	unsigned int row = 0;
	NODE5*	node;
	//char	prompt[] = {'+', '-', '*'};

	if (!pt->root) {
		printf("** Empty.\n");
		return;
	}

	fp = fopen (fname, "w");
	if (fp == NULL) {
		printf ("File(%s) creating error!\n", fname);
		return;
	}	
	printf ("Writing data to the %s...\n", fname);

	node = pt->root;
	//첫번째 리프 노드 찾음
	while (!node->is_leaf)  //트리 높이만큼 반복
		node = (NODE5*)node->pointers[0];

	while (true) {		
		for (i = 0; i < node->num_keys; i++) {
			row++;
			//소스 문장
			_fio_export_data (fp, pi_src, node->keys[i]);
			//번역 문장
			_fio_export_data (fp, pi_trg, node->pointers[i]);
			fputc ('\n', fp);
		}
		printf ("%c(line)%u: ", CR, row);  //라인 수

		if (node->pointers[pt->order - 1])
			node = (NODE5*)node->pointers[pt->order - 1];	//다음 리프 노드
		else break;		
	} //while	

	fclose (fp);
	printf ("%u rows have written.\n", row, fname);
	printf ("\n");
}

//교정(revision) 단어를 파일(txt)에서 입력(읽기)
int fio_import_revision (char *fname, BTREE* rs)
{
	FILE	*fp;
	char	row[ASIZE], rows[2][ASIZE], *rowp;
	int		length, skip=0;
	int		rown=0, cnt=0, idx, ins_cnt=0;

	printf ("Reading data from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Not exist!\n");
		return 0;
	}

	//행단위(개행문자까지) 읽음, 실패시 NULL 반환
	while (rowp = fgets (row, ASIZE, fp)) {		
		skip = 0;
		rowp = str_trim (rowp);	//개행(\n)은 여기서 잘림
		length = str_len (rowp) - 1;
		if (*(rowp+length) == '.') *(rowp+length) = '\0'; //맨마지막 마침표 제거

		if (length < 0) skip++;
		if (*rowp == '/') skip++;	//주석
			
		if (!skip) {
			idx = cnt % 2;
			str_copy (rows[idx], rowp);
			cnt++;
			if (idx) 
				if (tw2_rev_word_insert (rs, rows)) ins_cnt++;
		}
		printf ("%c(line)%u: ", CR, ++rown);  //라인 수
	}

	fclose (fp);
	printf ("%d have saved.\n", ins_cnt);

	return ins_cnt;	//저장개수
}

//교정(revision) 단어를 파일(txt)에 출력(저장)
void fio_export_revision (char* fname, BTREE* rs) 
{
	FILE*	fp;
	register int i;
	unsigned int row = 0;
	NODE5*		 node;

	if (!rs->root) {
		printf("** Empty.\n");
		return;
	}

	fp = fopen (fname, "w");
	if (fp == NULL) {
		printf ("File(%s) creating error!\n", fname);
		return;
	}	
	printf ("Writing data to the %s...\n", fname);

	node = rs->root;
	//첫번째 리프 노드 찾음
	while (!node->is_leaf)  //트리 높이만큼 반복
		node = (NODE5*)node->pointers[0];

	while (1) {		
		for (i = 0; i < node->num_keys; i++) {
			row++;
			//rs->outkey (node->keys[i]);
			fputs ((char*)node->keys[i], fp);
			fputc ('\n', fp);
			//printf (" >> ");
			//rs->outdata (node->pointers[i]);
			fputs ((char*)node->pointers[i], fp);
			fputc ('\n', fp);
			fputc ('\n', fp);
		}
		printf ("%c(line)%u: ", CR, row);

		if (node->pointers[rs->order - 1])
			node = (NODE5*)node->pointers[rs->order - 1];	//다음 리프 노드
		else break;
	} //while
	fclose (fp);
	printf ("%u rows have written to the file.(%s)\n", row, fname);
	printf ("\n");
}

//키를 하나씩 분리하여 단어 단위로 검색
int _fio_trans_key_each (FILE *fp, char* keys, BTREE** hb[], BTREE** wi, int mode, int kcnt)
{
	int		i=0, cnt=0, idx;
	unsigned int h, *akey2;
	BTREE	*t1;
	NODE5*	leaf;
	char	akey[ASIZE], *pkey, sbuf[SSIZE];

	akey2 = alloca (sizeof(unsigned int)); //스택에 할당(자동해제됨)
	if (!akey2) {
		printf ("## Failure to allocate alloca in _tw1_trans_search_each().\n");
		return -1;  //메모리 할당 실패
	}

	sbuf[0] = '\0';
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
			if (_fio_export_buffer (sbuf, wi[!mode], leaf->pointers[idx]) > 0) {
				cnt++;
			}
		} else str_cat (sbuf, "-");	//키값 없음
		str_cat (sbuf, "/");
	} //while

	str_cat (sbuf, ".\n");
	fputs (sbuf, fp);	//파일에 저장

	return cnt;
}

//파일에 있는 문장 번역(영한 mode에 상관없이 번역)
int fio_translation (char *fname, char *fname2, BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, int flag)
{
	FILE	*fp, *fp2;
	char	rows[SSIZE], keys[SSIZE], *rowp;
	register int i=0;
	int		ch, rown=0, kcnt, trans_cnt=0;
	unsigned int h;
	BTREE*	t1;
	void*	data;

	printf ("File (%s >> %s) Translating...\n", fname, fname2);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Input file(%s) not exist!\n", fname);
		return 0;
	}

	fp2 = fopen (fname2, "w");
	if (fp2 == NULL) {
		printf (", Output file(%s) creating error!\n", fname2);
		return 0;
	}

	//문장 단위로 읽음
	ch = fgetc (fp);
	while (ch != EOF) 
	{
		while (ch != '\n' && ch != EOF && i < SSIZE-2) {	//문장끝
			rows[i++] = ch;
			if (um_end(ch)) break;	//문장끝 부호(. ? !)
			ch = fgetc (fp);			
		}
		if (i >= SSIZE-2) break;

		rows[i] = '\0';
		rowp = str_trim_left (rows);
		if (*rowp == '/' && *(rowp+1) == '/') {	//주석			
			fputs (rowp, fp2);	//원문출력
			fputc ('\n', fp2);
			ch = fgetc (fp);
			i = 0;
			continue;
		}

		if (ch == '\n') {
			rows[i++] = ' ';
			//fputc ('\n', fp2);
		}
		ch = fgetc (fp);
		if (ch != EOF && !um_whites(ch)) continue;	//문장끝이 아님으로 판단하고 계속 읽음

		rows[i] = '\0';
		rowp = str_trim (rows);
		mode = str_is_eng_kor (rowp);	//0:영문, 1:한글, -1:특수문자(개행문자도 특수문자로 취급)
		if (mode >= 0) {
			fputs (rowp, fp2);	//원문출력
			fputc ('\n', fp2);

			//문장을 인덱스키로 변환
			str_copy (keys, rowp);
			if ((kcnt = tw1_words_input ("", keys, ws[mode], wi[mode], rs, qk[mode], FLAG_NONE)) > 0) {
				//printf (">> %s\n", keys);
				//해시값으로 번역키 B+트리 선택
				h = hash_value (keys);
				t1 = hb[mode][h];
				data = bpt_search (t1, keys);	//번역키				
				if (data) {	
					//번역 문장 보관용 스택에 입력
					tw2_stack_push (rowp);

					///번역정보(완전일치)를 파일에 출력
					if (_fio_export_data (fp2, wi[!mode], data) > 0) trans_cnt++;
					else tw2_stack_pop ();	//스택에 저장된 번역대상 문장 제거

				} else {
					//완전일치키가 없을때, 번역키를 하나씩 분리하여 단어단위로 검색하여 출력
					if (_fio_trans_key_each (fp2, keys, hb, wi, mode, kcnt) > 0) trans_cnt++;				
				}
			}
			fputc ('\n', fp2);
		}
		printf ("%c*line: %u: ", CR, ++rown);  //라인 수
		i = 0;

		if (flag==FLAG_NONE && trans_cnt==TRANS_CNT) {	//무료
			//파일에 출력
			fputs ("** 문장번역 개수를 제한합니다.\n", fp2);
			fputs ("** 유료 회원으로 등록하시면 제한없이 번역합니다.\n\n", fp2);
			//화면에 출력
			printf ("** 문장번역을 %d개로 제한합니다.\n", TRANS_CNT);
			printf ("** 유료 회원으로 등록하시면 제한없이 번역합니다.\n\n");
			break;
		}
	} //while

	fclose (fp2);
	fclose (fp);
	return trans_cnt;	//번역한 문장 수
}

//도움말 파일을 읽어서 출력
bool fio_read_help (char *fname)
{
	FILE	*fp;
	char	row[SSIZE], *rowp;

	//printf ("Reading data from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		//printf (", Not exist!\n");
		return false;
	}

	//행단위(개행문자까지) 읽음, 실패시 NULL 반환
	while (rowp = fgets (row, SSIZE, fp)) {
		if (*rowp == '/' && *(rowp+1) == '/') {
			printf ("%s", (rowp+2));
			getchar ();
		} else {
			printf ("%s", rowp);
		}
	}
	
	fclose (fp);
	return true;
}

//경로 생성
int fio_mkdir (char *dir)
{
	int iret;

	#ifdef __LINUX
		if ((iret = mkdir (dir, 0755)) >= 0) 
	#else
		if ((iret = mkdir (dir)) >= 0) 
	#endif
			printf ("** %s directory created.\n", dir);
		//else
			//printf ("** %s directory creating failed.\n", dir);
	return iret;
}

//사용자 작업경로에 파일명 입력(파일명 길이 반환)
int fio_getchar_fname (char* msg, char* fname)
{
	register int i;
	int c;
	char *pstr = fname;

	pstr = (char*)getcwd (NULL, 0);	//Current Directory
	str_replace (pstr, '\\', '/');
	*pstr = a_upper (*pstr);
	printf ("(Path)%s", pstr);

	str_copy (fname, DIR_WORKS);
	printf ("%s\n", fname+1);

	printf (msg);

	i = str_len (fname);
	while ((c = getchar()) != (int)'\n' && i < ASIZE-2)
		fname[i++] = c;
	fname[i] = '\0';

	if (i == ASIZE-2) return 0;

	str_trim (fname);
	return (str_cmp (DIR_WORKS, fname)) ? i : 0;
}

//유료회원 개인키 저장 (암호화)
int fio_write_member_key (char* fname, char* skey)
{
	FILE	*fp;

	fp = fopen (fname, "w");
	if (fp == NULL) {
		printf ("File(%s) creating error!\n", fname);
		return 0;
	}
	_fio_write_header (fp, fname);  //파일 헤더 쓰기

	fputs (skey, fp);

	fclose (fp);
	return 1;
}

//유료회원 개인키 읽기 (복호화)
int fio_read_member_key (char* fname, char* skey)
{
	FILE *fp;
	char buf[ASIZE];

	fp = fopen (fname, "r");
	if (fp == NULL) {
		//printf ("File(%s) reading error!\n", fname);
		return 0;
	}
	if (_fio_read_header (fp, fname, buf) < 0) {
		//printf (", Header error!\n");
		return 0;
	}

	fgets (skey, SSIZE, fp);

	fclose (fp);
	return 1;
}

//번역 데이터 파일(txt)에서 CAPTION 문장들을 읽어서 저장(입력)
//mode 구분, 0:영문(한글번역), 1:한글(영문번역)
int fio_import_caption (char *fname, BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk)
{
	FILE	*fp;
	register int i=0;
	char	ch, row[SSIZE], rows[2][SSIZE], *rowp;
	char	caps[2][ASIZE], capn[ASIZE];
	int		idx, skip=0, ie;
	int		rown=0, cnt=0, cnt_sum=0;
	bool	brun, bcap;

	printf ("Reading data from the %s...\n", fname);
	fp = fopen (fname, "r");
	if (fp == NULL) {
		printf (", Not exist!\n");
		return 0;
	}

	rows[0][0] = '\0';
	rows[1][0] = '\0';

	//개행문자, 문장 단위로 읽음
	ch = fgetc (fp);
	while (ch != EOF) 
	{
		ie = 0;
		while (ch != '\n' && ch != EOF && i < SSIZE-2) {	//문장끝
			row[i++] = ch;
			if (um_end(ch)) ie = 1;	//문장끝 부호(. ? !)가 있는지 여부
			ch = fgetc (fp);			
		}
		if (i >= SSIZE-2) break;

		row[i] = '\0';
		rowp = str_trim_left (row);
		if (*rowp == '/' && *(rowp+1) == '/') {	//주석
			ch = fgetc (fp);
			i = 0;
			continue;
		}
		if (ch == '\n')	row[i++] =  ' ';
		ch = fgetc (fp);
		if (ch != EOF  && !ie) continue;	//문장끝이 아님으로 판단하고 계속 읽음

		row[i] = '\0';
		rowp = str_trim (row);

		//0:영문, 1:한글, -1:특수문자
		idx = str_is_eng_kor (rowp);
		if (idx >= 0) {
			//캡션 문장인가?
			bcap = (*rowp == '*') ? true : false;

			str_copy (rows[idx], rowp);
			if (bcap) str_copy (caps[idx], rowp+1);

			brun = (mode) ? !idx: idx;	//0:영문(한글번역), 1:한글(영문번역)
			if (brun && *rows[mode]) {
				if (bcap) {
					tw2_insertion_caption (ws, wi, mode, rs, qk, caps);	//캡션 문장 저장
					idx = str_len (caps[0]) - 1;
					caps[0][idx] = ' ';		//마침표 제거
					caps[0][idx+1] = '\0';
					idx = str_len (caps[1]) - 1;
					caps[1][idx] = ' ';		//마침표 제거
					caps[1][idx+1] = '\0';

					cnt = 0;

				} else {
					tw1_insertion_from_file (ws, wi, hb, mode, rs, qk, rows, -1);	//저장 문장 수 누적

					//캡션을 문장 머리에 붙여 저장					
					str_copy (row, caps[0]);
					str_cat (row, uint_to_str (cnt, capn));
					str_cat (row, ": ");
					str_cat (row, rows[0]);
					str_copy (rows[0], row);
					
					str_copy (row, caps[0]);
					str_cat (row, uint_to_str (cnt, capn));
					str_cat (row, ": ");
					str_cat (row, rows[1]);
					str_copy (rows[1], row);

					//캡션 문장은 해시값을 0으로 정함.
					cnt += tw1_insertion_from_file (ws, wi, hb, mode, rs, qk, rows, 0);
					cnt_sum++;
				}
			}
		}
		printf ("%c*line: %d/%d: ", CR, cnt_sum, ++rown);  //라인 수
		i = 0;
	} //while

	fclose (fp);
	printf ("%d rows have saved.\n", cnt_sum);

	return cnt_sum;	//저장한 문장 수
}
