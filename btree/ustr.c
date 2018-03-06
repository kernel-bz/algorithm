//
//  Source: ustr.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-01-05 문자열 처리 함수들을 작성하다.
//		2011-02-24 문자열로 해시값을 산출하는 함수(hash_value)를 추가하다.
//		yyyy-mm-dd ...
//

#include "dtype.h"
#include "umac.h"

//문자열 길이
unsigned int str_len (char* t)
{
	char *s = t;
	while (*t++);
	t--;
	return (unsigned int)(t - s);
}

//문자열 추가(t <-- s)
void str_cat (char *t, char *s)
{ 
	while (*t++);			//t의 끝위치
	t--;
	while (*t++ = *s++);	//복사
} 

//문장부호 인가?
int is_smark (char c)
{
	int mark[] = {33, 34, 44, 58, 59, 63, 96};	//! " , : ; ? ` ('는 단어줄임에 있으므로 제외)
	register int i;

	for (i = 0; i < sizeof(mark)/sizeof(int); i++)
		if (c == mark[i]) return 1;
	return 0;
}

//문장 끝인가?
int is_end (char c)
{
	return (c==46 || c==63 || c==33) ? 1 : 0;	//. ? !
}

//whitespace 인가?
int is_whitespace (char c)
{
	return (c > 0 && c < 33) ? 1 : 0;  //한글은 음수, 2바이트(0x80,0x80)
}

//whitespace 혹은 문장부호 인가?
int is_space_mark (char c)
{
	return (c > 0 && c < 33) ? 1 : is_smark (c);
}

//숫자인가?
int is_digit (char c)
{
	return (c > 47 && c < 58) ? 1 : 0;
}

///unsigned int를 문자열로 변환
char* uint_to_str (unsigned int n, char *s)
{
	register int i=0, j=0;
	char temp;

	do
		s[i++] = n - (n / 10 * 10) + '0';	//숫자를 문자 아스키값으로
	while (n /= 10);
	s[i] = '\0';
	i--;

	while (i > j) {		//숫자 문자열 순서바꿈(reverse)
		temp = s[i];
		s[i--] = s[j];
		s[j++] = temp;
	}
	return s;
}

//unsigned int를 문자열로 변환후, 매개변수로 주어진 길이가 되도록 왼쪽에 '0'을 패딩
char* uint_to_str_len (unsigned int n, char *s, unsigned int length)
{
	register int i, gap;
	char sz[] = {"0000000000"}; //10자리(unsigned int)
	char *ps, *ps2;

	uint_to_str (n, s);
	ps = s;  //처음 위치 포인터
	ps2 = s;
	gap = length - str_len (s);
	if (gap > 0) {
		for (i = 0; i < gap; i++);  //패딩할 위치까지 이동
		while (sz[i++] = *s++);
		i = 0;
		while (*ps++ = sz[i++]);
	}
	return ps2;
}

//문자열을 unsigned int로 변환
unsigned int str_to_uint (char *s) 
{ 
	unsigned int n = 0;	
	//while (*s >= '0' && *s <= '9')
	while (*s) {
		if (*s > 47 && *s < 58)	 n = 10 * n + (*s - '0');
		s++;
	}
	return n;
} 

//하나의 아스키 문자를 소문자로
char a_lower (char c) 
{ 
	return (c >= 'A' && c <= 'Z') ? c + ('a' - 'A') : c;
} 

//하나의 아스키 문자를 대문자로
char a_upper (char c) 
{ 
	return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
} 

//문자열을 소문자로
char* str_lower (char *t)
{
	char* s = t;

	while (*t) {
		if (*t >= 'A' && *t <= 'Z') 
			*t = *t + 'a' - 'A';
		t++;
	}
	return s;
} 

//알파벹인가?
int is_alpha (char c)
{
	c = a_lower (c);
	return (c > 96 && c < 123) ? 1 : 0;
}

//문자열 순서 바꿈(reverse)
void str_reverse (char *s)
{
	register int i, j;
	char temp;

	i = str_len (s) - 1;
	j = 0;
	while (i > j) {
		temp = s[i];
		s[i--] = s[j];
		s[j++] = temp;
	}
	//return s;
}

//문자열 복사
void str_copy (char *t, char *s) 
{ 
	while (*t++ = *s++);
} 

//문자열 비교
int str_cmp (char *s, char *t) 
{ 
	for ( ; *s == *t; s++, t++) 
		if (*s == '\0' || *t == '\0') break;
	return *s - *t;
}

//문자열 s까지 일지하는지 비교
int str_cmp_like (char *s, char *t) 
{ 
	for ( ; *s == *t; s++, t++) ;
	if (*s == '\0') return 0; 
	return *s - *t;
}

//문자열의 구성요소(숫자키 인덱스)를 unsigned int로 변환후 비교
int str_cmp_int (char *s, char *t) 
{ 
	register int i;
	char sa[11], ta[11];  //배열, unsigned int의 숫자개수 만큼의 길이
	unsigned int su=0, tu=0;

	while (*s && *t) {
		i = 0;
		while (*s != '_') sa[i++] = *s++;
		sa[i] = '\0';
		su = str_to_uint (sa);
		s++;
		
		i = 0;
		while (*t != '_') ta[i++] = *t++;
		ta[i] = '\0';
		tu = str_to_uint (ta);
		t++;

		if (su != tu) break;
	}

	//unsigned int는 음수가 없으므로 if로 비교
	if (su < tu) return -1;
	if (su > tu) return 1;
	return *s - *t;
}

//문자열의 구성요소(숫자키 인덱스)를 unsigned int로 변환후 비교
//*s 까지 일치하는지 비교
int str_cmp_int_like (void *p1, void *p2) 
{ 
	char *s, *t;
	register int i;
	char sa[11], ta[11];  //배열, unsigned int의 숫자개수 만큼의 길이
	unsigned int su, tu;

	s = (char*)p1;
	t = (char*)p2;	

	while (*s && *t) {
		i = 0;
		while (*s != '_') sa[i++] = *s++;
		sa[i] = '\0';
		su = str_to_uint (sa);
		s++;
		
		i = 0;
		while (*t != '_') ta[i++] = *t++;
		ta[i] = '\0';
		tu = str_to_uint (ta);
		t++;

		if (su != tu) break;
	}

	if (*s && !*t) return 1;

	//unsigned int는 음수가	없으므로 if로 비교
	if (su < tu) return -1;  
	if (su > tu) return 1;
	return 0;
}

//문자열 t안에 문자열 s가 포함되어 있으면 0을 리턴
int str_ncmp (char *t, char *s, int num)
{
	register int cnt = 0;

	for ( ; *s != *t; t++) 
		if (*t == '\0') return 1; 

	for ( ; *s == *t; s++, t++) {
		if (++cnt == num) return 0;
		if (*s == '\0') return 1;
	}

	return 1;
}

/* 번역키는 문자열을 숫자로 변환후 비교해야 함
int str_cmp_int_similar (void *p1, void *p2) 
{ 
	return str_ncmp ((char*)p2, (char*)p1, str_len ((char*)p1));
}
*/
///문자열의 구성요소(숫자키 인덱스)를 unsigned int로 변환후 비교
///단어가 2개 이상 일치하는지 여부
///-1을 리턴하면 리프 검색 못함
int str_cmp_int_similar (void *p1, void *p2) 
{ 
	char *s, *t;
	register int i, j=0, sc=0, tc=0;
	char sa[11], ta[11];  //배열, unsigned int의 숫자개수 만큼의 길이
	unsigned int su=0, tu=0;
	unsigned int sca[SSIZE], tca[SSIZE];

	s = (char*)p1;
	t = (char*)p2;	

	while (*t) {
		i = 0;
		while (*t != '_') ta[i++] = *t++;
		ta[i] = '\0';
		tu = str_to_uint (ta);
		t++;
		tca[tc++] = tu;
	} //while
	//if (tc == 0) return 1;

	while (*s) {
		i = 0;
		while (*s != '_') sa[i++] = *s++;
		sa[i] = '\0';
		su = str_to_uint (sa);
		s++;
		sca[sc++] = su;
	}

	if (sc > tc) return 1;

	for (i = 0; i < tc; i++) {
		if (tca[i] == sca[j]) j++;
		if (j == sc) return 0;
	}

	return 1;
}

//문자열 앞뒤의 whitespace 잘라냄
char* str_trim (char *s)
{
	char *t;

	//t = s + str_len (s) - 1;
	t = s + str_len (s);
	*t-- = '\0';
	while (um_whites (*t) ) *t-- = '\0';  //뒤부분 whitespace 제거
	while (um_whites (*s) ) s++;  //앞부분 whitespace 제거

	return s;
}

//문자열의 왼쪽 whitespace 잘라냄
char* str_trim_left (char *s)
{
	while (um_whites (*s) ) s++;  //앞부분 whitespace 제거
	return s;
}

//문자열에서 특정 문자 교체
void str_replace (char *s, char c1, char c2) 
{ 
	while (*s) {
		if (*s==c1) *s = c2;
		s++;
	}
} 

//문자열 s로 해시값 산출(sh는 이미 정한 해시값)
unsigned int hash_value (char *s) 
{ 
	unsigned int h;

	for (h = 0; *s != '\0'; s++)	//문자열 s의 길이만큼 반복
		h = *s + HASH * h;			//문자 *s의 아스키값에 해시값 더함

	return (h % (HASHSIZE-1)) + 1;	//(0 < 해시값 < HASHSIZE), 0은 캡션 문장에서 사용
} 

//한글인가?
bool is_kor (char* s)
{
	const char *sk1 = "가";	//0xB0A1
	const char *sk2 = "힝";	//0xC8FE

	if (*s >= sk1[0] && *s <= sk2[0])
		if (*(s+1) >= sk1[1] && *(s+1) <= sk2[1]) return true;
	return false;
}

//영문자인가?
bool is_eng (char* s)
{
	return (*s >= 32 && *s <= 126) ? true : false;	//32는 공백
}

//문자열에 한글이 포함되어 있는가?
bool str_is_kor (char* s)
{
	while (*s) {
		if (is_kor (s)) return true;	//한글 있음
		s++;
	}
	return false;	//한글 없음
}

//문자열이 모두 영문자인가?
bool str_is_eng (char* s)
{
	while (*s) {
		if (*s < 32 || *s > 126) return false;	//영문자 아님(특수문자)
		s++;
	}	
	return true;	//모두 영문자
}

//문자열이 영문 또는 한글인가?
int str_is_eng_kor (char* s)
{
	int cnt=0;

	if (!*s) return -1;

	while (*s) {
		if (*s > 126) return -1;	//특수문자
		if (*s < 32) {
			if (is_kor (s)) {
				s++;
				cnt++;
			} else return -1;	//특수문자
		}
		s++;
	}	
	return (cnt) ? 1 : 0;	//0:모두 영문자, 1:한글있음
}
