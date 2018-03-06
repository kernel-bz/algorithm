//
//  Source: tw2.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2011, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-02-11 tw2.c 모듈을 코딩하다.
//		2011-02-16 단어 수정 함수(tw2_word_update) 추가하다.
//		2011-02-22 파일안의 문장 번역 추가하다.(tw2_file_translation: 파일입력 >> 번역 >> 파일출력)
//		yyyy-mm-dd ...
//

#include <stdio.h>
#include <stdlib.h>	//malloc

#include "dtype.h"
#include "ustr.h"		//문자열
#include "bpt3.h"
#include "tw1.h"
#include "fileio.h"
#include "utime.h"
#include "stack.h"

//교정(revision) 단어 B+트리에 입력
bool tw2_rev_word_insert (BTREE* rs, char akey[][ASIZE])
{
	char *pkey1, *pkey2, *mkey1, *mkey2;
	NODE5 *leaf;

	pkey1 = str_lower (akey[0]);	//소문자로 변환
	pkey1 = str_trim (pkey1);		//앞뒤 whitespace 잘라냄
	if (! *pkey1) return false;

	pkey2 = str_lower (akey[1]);	//소문자로 변환
	pkey2 = str_trim (pkey2);		//앞뒤 whitespace 잘라냄
	if (! *pkey2) return false;
	
	if (bpt_find_leaf_key (rs, pkey1, &leaf) < 0) {	
		//리프에 동일한 키가 없다면 메모리할당후 삽입
		mkey1 = malloc (str_len (pkey1) + 1);
		if (!mkey1) {
			printf ("## Failure to allocate memory.\n");
			return false;
		}
		str_copy (mkey1, pkey1);

		mkey2 = malloc (str_len (pkey2) + 1);
		if (!mkey2) {
			printf ("## Failure to allocate memory.\n");
			return false;
		}
		str_copy (mkey2, pkey2);
		//B+트리에 입력
		rs->root = bpt_insert (rs, leaf, mkey1, mkey2, FLAG_INSERT);
		//printf ("** Inserted.\n");
		return true;

	} else {
		printf ("** Exist same key.\n");
		return false;
	}
}

//교정(revision) 단어 키보에서 입력
int tw2_rev_word_input (BTREE* rs)
{
	char akey[2][ASIZE];
	int	 c, cnt=0;
	register int i;

	while (1) {
		printf ("Word1: ");
		i = 0;
		while ((c = getchar()) != (int)'\n' && i < ASIZE-2)	akey[0][i++] = c;
		akey[0][i] = '\0';
		if (i < 2 || i == ASIZE-2) break;

		printf ("Word2: ");
		i = 0;
		while ((c = getchar()) != (int)'\n' && i < ASIZE-2)	akey[1][i++] = c;
		akey[1][i] = '\0';
		if (i < 2 || i == ASIZE-2) break;

		if (tw2_rev_word_insert (rs, akey)) cnt++;
	}
	return cnt;
}

///교정(revision) 단어 사전에서 삭제
int tw2_rev_word_delete (BTREE* rs)
{
	char akey1[ASIZE], *pkey1;
	int	 c, cnt=0;
	register int i;
	bool deleted;

	while (1) {
		printf ("Word1: ");
		i = 0;
		while ((c = getchar()) != (int)'\n' && i < ASIZE-2)
			akey1[i++] = c;
		akey1[i] = '\0';

		if (i < 2 || i == ASIZE-2) break;
		pkey1 = str_lower (akey1);	//소문자로 변환
		pkey1 = str_trim (pkey1);	//앞뒤 whitespace 잘라냄
		if (! *pkey1) break;

		//리프노드로 이동하여 삭제
		//deleted = false;
		rs->root = bpt_delete (rs, pkey1, &deleted);
		if (deleted) {
			printf ("** Deleted.\n");
			cnt++;
		}
	} //while
	return cnt;
}

//교정(revision) 단어 입력(파일)
int tw2_rev_word_import (BTREE* rs) 
{
	char fname[ASIZE];
	if (! fio_getchar_fname ("Import(txt) FileName: ", fname)) return 0;
	return fio_import_revision (fname, rs);	//저장개수 반환
}

//교정(revision) 단어 출력(파일)
void tw2_rev_word_export (BTREE* rs) 
{
	char fname[ASIZE];
	if (! fio_getchar_fname ("Export(txt) FileName: ", fname)) return;
	fio_export_revision (fname, rs);
}

///수정 실행(삭제된 고유번호로 입력됨, 고유번호 변환 없음)
unsigned int _tw2_word_update_run (BTREE* ws, BTREE* wi, char *in_word, unsigned int kno)
{
	NODE5	*leaf_ps, *leaf_pi;
	char*	pword;
	unsigned int *pno;  //입력용 일련번호(고유숫자)
	int		err=0;

	if (ws->kno != wi->kno)	{	//인덱스 고유번호 다름(이조건이 발생하면 않됨)
		printf ("## Failure to index serial number.(ws:%u, wi:%u)\n", ws->kno, wi->kno);	
		return UIFAIL;
	}

	//삭제된 후 리프 다시 검색
	if (bpt_find_leaf_key (ws, in_word, &leaf_ps) >= 0) {
		printf ("## Exist same word.\n");
		return UIFAIL;
	}

	//리프에 동일한 키가 없다면 메모리할당후 삽입
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
	*pno = kno;

	if (bpt_find_leaf_key (wi, pno, &leaf_pi) >= 0) {
		//고유번호 존재 (이조건이 발생하면 않됨)
		printf ("## Exist same number key.(s:%s, i:%u)\n", in_word, *pno);
		err++;
	}

	if (err) {
		free (pword);
		free (pno);
		return UIFAIL;
	}

	//문자키 입력
	ws->root = bpt_insert (ws, leaf_ps, pword, pno, FLAG_UPDATE);
	//순자키 입력
	wi->root = bpt_insert (wi, leaf_pi, pno, pword, FLAG_UPDATE);

	return *pno;
}

///단어 수정(삭제후 입력)
//현재, 교정 단어로 수정되면 번역하지 못함
int tw2_word_update (BTREE* ws, BTREE* wi, char* prompt)
{
	char akey1[ASIZE], akey2[ASIZE];
	char *pkey1, *pkey2;
	int	 c, cnt=0;
	register int i;
	unsigned int kno;
	NODE5 *leaf;
	bool deleted;

	while (1) {
		printf ("%s Word: ", prompt);
		i = 0;
		while ((c = getchar()) != (int)'\n' && i < ASIZE-2)
			akey1[i++] = c;
		akey1[i] = '\0';

		if (i < 2 || i == ASIZE-2) break;
		pkey1 = str_lower (akey1);	//소문자로 변환
		pkey1 = str_trim (pkey1);	//앞뒤 whitespace 잘라냄
		if (! *pkey1) break;

		printf ("%s To: ", prompt);
		i = 0;
		while ((c = getchar()) != (int)'\n' && i < ASIZE-2)
			akey2[i++] = c;
		akey2[i] = '\0';

		if (i < 2 || i == ASIZE-2) break;
		pkey2 = str_lower (akey2);	//소문자로 변환
		pkey2 = str_trim (pkey2);	//앞뒤 whitespace 잘라냄
		if (! *pkey2) break;

		if (bpt_find_leaf_key (ws, pkey2, &leaf) >= 0) {
			printf ("*# Exist same word.\n");	//이 조건이 발생하면 번역키도 수정해야 함(현재는 그냥 종료)
			break;
		}

		//수정(삭제후 입력)
		kno = tw1_drop_word_run (ws, wi, pkey1, &deleted);
		if (deleted) {
			//수정(입력)
			if (_tw2_word_update_run (ws, wi, pkey2, kno) != UIFAIL) {
				printf ("** Updated.\n");
				cnt++;
			}
		} else printf ("** Not exist.\n");

		printf ("\n");
	} //while

	return cnt;
}

//파일에서 번역된 문장들 입력
int tw2_import_from_file (BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, int flag)
{
	char fname[ASIZE];
	int c;
	double	msec1, msec2;

	if (! fio_getchar_fname ("Import(txt) FileName: ", fname)) return 0;

	//미리 초 단위(시작)
	msec1 = time_get_msec ();

	if (flag == FLAG_CAP)
		c = fio_import_caption (fname, ws, wi, hb, mode, rs, qk);
	else
		c = fio_import (fname, ws, wi, hb, mode, rs, qk);

	//미리 초 단위(종료)
	msec2 = time_get_msec ();
	//실행시간
	printf ("** Run Time: %.3f Secs\n\n", msec2 - msec1);

	return c;
}

//문장들을 파일에 출력(k=0: 단어, k=2: 문장)
void tw2_export_to_file (BTREE** hb[], BTREE** wi, int mode, int sh)
{
	char fname[ASIZE], *pfname, buf[ASIZE];
	register int i = 0, h;
	double	msec1, msec2;

	if (!(i = fio_getchar_fname ("Export(txt) FileName: ", fname))) return;

	//미리 초 단위(시작)
	msec1 = time_get_msec ();

	if (sh == 0) {
		//캡션 문장 출력
		fio_export (fname, hb[mode][sh], wi[mode], wi[!mode]);
	} else {
		fname[i] = '_';
		pfname = fname;
		for (h = 1; h < HASHSIZE; h++) {		
			fname[i+1] = '\0';
			str_cat (pfname, uint_to_str (h, buf));	//파일명 끝에 해시번호 추가
			fio_export (pfname, hb[mode][h], wi[mode], wi[!mode]);
		}
	}
	//미리 초 단위(종료)
	msec2 = time_get_msec ();
	//실행시간
	printf ("** Run Time: %.3f Secs\n\n", msec2 - msec1);
}

//파일안의 문장들 번역
void tw2_file_translation (BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, int flag)
{
	char fname[ASIZE], fname2[ASIZE];
	int cnt;
	double	msec1, msec2;

	if (! fio_getchar_fname ("Input (txt) FileName: ", fname)) return;
	if (! fio_getchar_fname ("Output(txt) FileName: ", fname2)) return;

	//미리 초 단위(시작)
	msec1 = time_get_msec ();

	cnt = fio_translation (fname, fname2, ws, wi, hb, mode, rs, qk, flag);	//번역한 문장개수 반환

	//미리 초 단위(종료)
	msec2 = time_get_msec ();
	//실행시간
	printf ("** %d Run Time: %.3f Secs\n\n", cnt, msec2 - msec1);
}

//해시값을 하나 입력받음
int tw2_getchar_hash_value (void)
{
	char value[ASIZE];
	register int i=0;
	int c;

	printf ("Input (0 <= h < %d): ", HASHSIZE);
	while ((c = getchar()) != (int)'\n' && i < ASIZE-2)
		value[i++] = c;
	value[i] = '\0';

	return (i==0) ? -1 : str_to_uint (value);
}

//개인키 암호화
int _tw2_getchar_encode_key (char* msg, char* akey, int flag)
{
	register int i=0, j;
	const int length = 60;	//키길이
	const int codec[] = { 0, 1, 0, 2, 5, 2, 0, 3, 3, 0, 7 }; //11개
	int c, k, asize = sizeof(codec) / sizeof(codec[0]);

	if (flag) {
		printf ("%s", msg);
		while ((c = getchar()) != (int)'\n' && i < length-2)
			akey[i++] = c;
		akey[i] = '\0';
	} else i = str_len (akey);

	//암호화
	for (j=0; j < i; j++) {
		k = j + asize;
		akey[j] -= (k * (codec[j%asize])) % 33;
		akey[j] += 20;
	}
	str_reverse (akey);
	akey[i] = 19;	//키의 끝

	//랜덤문자로 키길이(length)까지 채움
	time_rand_seed_init ();	
	for (j = i+1; j < length; j++)
		akey[j] = time_random_between (33, 126);	//출력가능한 아스키문자
	akey[j] = '\0';
	
	return i;	//입력받은 키의 길이
}

//개인키 복호화
void _tw2_get_decode_key (char* skey, char* out_key, char* out_key_time, int cnt, int flag)
{
	char akey[ASIZE], *pkey;
	register int i, j;
	const int length = 60;	//키길이
	const int codec[] = { 0, 1, 0, 2, 5, 2, 0, 3, 3, 0, 7 }; //11개
	int k, asize = sizeof(codec) / sizeof(codec[0]);
	char *str[] = {"key  ", "phone", "email", "time "};

	while (cnt--) {
		pkey = akey;
		i = 0;
		while (*skey != 19 && i++ < ASIZE-2) *pkey++ = *skey++;
		*pkey = '\0';

		//복호화
		str_reverse (akey);
		for (i=0; i < (int)str_len (akey); i++) {
			k = i + asize;
			akey[i] += (k * (codec[i%asize])) % 33;
			akey[i] -= 20;
		}
		//akey[i] = '\0';
		if (flag) printf ("%s: %s\n", str[cnt], akey);
		if (cnt==0) str_copy (out_key, akey);
		if (cnt==3) str_copy (out_key_time, akey);

		//키길이(length)까지 채워진 랜덤문자
		for (j = i; j < length; j++) *skey++;
	}
}

//개인키 입력 (암호화)
int tw2_member_key_input (char *fname, int flag)
{
	char skey[SSIZE], akey[ASIZE];
	int	 kc;

	skey[0] = '\0';

	//생성일시, 현재시간(초단위)
	if (flag == FLAG_YES) {
		kc = _tw2_getchar_encode_key ("", uint_to_str (time_get_sec (), akey), FLAG_NONE);
		str_cat (skey, akey);

		//이메일 주소
		kc = _tw2_getchar_encode_key ("Input email: ", akey, FLAG_INSERT);
		str_cat (skey, akey);

		//전화번호
		kc = _tw2_getchar_encode_key ("Input phone: ", akey, FLAG_INSERT);
		str_cat (skey, akey);

		//개인키(암호)
		kc = _tw2_getchar_encode_key ("Input   key: ", akey, FLAG_INSERT);
		str_cat (skey, akey);

	} else {
		kc = _tw2_getchar_encode_key ("", uint_to_str (UIFAIL, akey), FLAG_NONE);	//유효기간 경과
		str_cat (skey, akey);
	}	

	//0: 개인키 입력 없음
	if (kc) {
		kc = fio_write_member_key (fname, skey);
		if (kc && flag == FLAG_YES) printf ("Private key have written.\n");
	}
	return kc;
}

//개인키 출력 (복호화)
void tw2_member_key_output (char *fname)
{
	char skey[SSIZE], akey[ASIZE], akey_time[ASIZE];

	if (fio_read_member_key (fname, skey)) 
		_tw2_get_decode_key (skey, akey, akey_time, 4, FLAG_VIEW);	//skey 복호화, 화면에 출력
}

//관리자 비밀번호를 입력받음
bool tw2_getchar_manager_pw (void)
{
	char cpw[ASIZE];
	register int i=0;
	int c;

	printf ("Input password: ");
	while ((c = getchar()) != (int)'\n' && i < ASIZE-2) 
		cpw[i++] = c;
	cpw[i] = '\0';
	if (i==0) return false;

	if (str_cmp (MANAGER, cpw)) return false;

	//화면 클리어(입력된 암호가 않보이도록)
	#ifdef __LINUX
		system ("clear");	
	#else
		system ("cls");
	#endif

	return true;
}

//유료회원 개인키 읽기(비교)
int tw2_getchar_member_pw (char *fname)
{
	char skey[SSIZE];
	char akey1[ASIZE], akey2[ASIZE], akey_time[ASIZE];
	register int i=0;
	unsigned int utime;
	int c, dtime;

	if (! fio_read_member_key (fname, skey)) return -1;
	//skey 복호화
	_tw2_get_decode_key (skey, akey1, akey_time, 4, FLAG_NONE);	//화면에 출력않함

	utime = (unsigned int)time_get_sec ();
	dtime = (utime - str_to_uint (akey_time)) / 3600 / 24;	//초를 하루단위로 환산
	printf ("* 개인키 남은 유효일: %d일/%d일\n", PKEY_DDAY - dtime, PKEY_DDAY);
	
	if (dtime > PKEY_DDAY) {
		tw2_member_key_input (FNAME_KEY, FLAG_NO);
		printf ("* 개인키 유효기간이 지났습니다. 재발급 신청해 주시기 바랍니다.\n\n");
		return -1;
	}	

	printf ("Input password: ");
	while ((c = getchar()) != (int)'\n' && i < ASIZE-2)
		akey2[i++] = c;
	akey2[i] = '\0';
	if (i==0) return 0;

	if (str_cmp (akey1, akey2)) return 0;

	//화면 클리어(입력된 암호가 않보이도록)
	#ifdef __LINUX
		system ("clear");	
	#else
		system ("cls");
	#endif

	return 1;
}

//CAPTION 을 rs 트리에 저장
int tw2_insertion_caption (BTREE** ws, BTREE** wi, int mode, BTREE* rs, QUEUE** qk, char rows[][ASIZE])
{
	char keys[2][ASIZE];	//숫자조합 번역키 배열
	int saved = 0, idx;

	str_copy (keys[mode], rows[mode]);	//행단위 문자열 문장1

	//숫자키로 변환 
	if ((idx = tw1_words_input (_tw1_prompt(mode), keys[mode], ws[mode], wi[mode], rs, qk[mode], FLAG_AUTO)) > 0) {
		str_copy (keys[!mode], rows[!mode]);	//행단위 문자열 문장2

		//번역키로 변환 성공 여부
		if ((idx = tw1_words_input (_tw1_prompt(!mode), keys[!mode], ws[!mode], wi[!mode], rs, qk[!mode], FLAG_AUTO)) > 0) {
			//rs 트리에 저장
			if (tw2_rev_word_insert (rs, keys)) saved = 1;
		}
	}
	return saved;
}

void tw2_stack_review (void)
{
	unsigned int i=0;
	char *ps;
	NODE2* sb;

	sb = StackTW->bottom;
	if (!sb) {
		printf ("** 번역된 문장이 없습니다.\n");
		return;
	}

	while (true) {
		if (!StackTW->bottom) StackTW->bottom = sb;	//계속 반복

		ps = (char*)stack_bottom (StackTW, FLAG_VIEW);
		printf ("%s\n", ps);

		time_sleep (str_len (ps) / 10);	//문장길이에 따라서 대기

		if (!(++i%8) && tw1_qn_answer ("* Would you like to see more? [Y/n] ", FLAG_YES) == FLAG_NO) break;
	}
	StackTW->bottom = sb;
}

//번역 문장 보관용 스택에 입력
int tw2_stack_push (char* str)
{
	char* sm;

	if (StackTW_Enable) {
		sm = malloc (str_len (str) + 2);
		if (!sm) {
			printf ("## StackTW memory allocation error!\n");
			return -1;
		}
		str_copy (sm, str);
		stack_push_limit (StackTW, sm, STACK_HEIGHT);	//전역 스택의 탑에 입력
		
		return 1;
	}
	return 0;
}

//스택에 저장된 번역대상 문장 제거
void tw2_stack_pop (void)
{
	if (StackTW_Enable) 
		stack_pop_top (StackTW, FLAG_DELETE);
}
