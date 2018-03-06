//
//  Source: ustr.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-12-18 문자열 처리 함수들을 작성하다.
//		2011-02-24 문자열로 해시값을 산출하는 함수(hash_value)를 추가하다.
//		yyyy-mm-dd ...
//

//unsigned int를 문자열로 변환
char* uint_to_str (unsigned int num, char *s);

//unsigned int를 문자열로 변환후, 매개변수로 주어진 길이가 되도록 왼쪽에 '0'을 패딩
char* uint_to_str_len (unsigned int n, char *s, unsigned int length);

//문장부호 인가?
int is_smark (char c);

//문장 끝인가?
int is_end (char c);

//whitespace 인가?
int is_whitespace (char c);

//whitespace 혹은 문장부호 인가?
int is_space_mark (char c);

//숫자인가?
int is_digit (char c);

//문자열을 unsigned int로 변환
unsigned int str_to_uint (char *s);

//하나의 아스키 문자를 소문자로
char a_lower (char c);

//하나의 아스키 문자를 대문자로
char a_upper (char c);

//문자열을 소문자로
char* str_lower (char *t);

//알파벹인가?
int is_alpha (char c);

//문자열 순서 바꿈(reverse)
void str_reverse (char *s);

//문자열 복사
void str_copy (char *t, char *s);

//문자열 추가
char* str_cat (char *t, char *s);

//문자열 길이
unsigned int str_len (char* t);

//문자열 비교
int str_cmp (char *s, char *t);

//문자열 s까지만 비교
int str_cmp_like (char *s, char *t);

//문자열의 구성요소(숫자키 인덱스)를 unsigned int로 변환후 비교
int str_cmp_int (char *s, char *t);

//문자열의 구성요소(숫자키 인덱스)를 unsigned int로 변환후 비교
//*s 길이 만큼만 비교
int str_cmp_int_like (void*, void*);

//단어가 2개 이상 일치하는지 여부
int str_cmp_int_similar (void*, void*);

///문자열 앞뒤의 whitespace 잘라냄
char* str_trim (char *s);

///문자열의 왼쪽 whitespace 잘라냄
char* str_trim_left (char *s);

//문자열에서 특정 문자 교체
void str_replace (char *s, char c1, char c2);


///문자열 s로 해시값 산출
unsigned int hash_value (char *s);


///한글인가?
bool is_kor (char* s);

///영문자인가?
bool is_eng (char* s);

///문자열에 한글이 포함되어 있는가?
bool str_is_kor (char* s);

///문자열이 모두 영문자인가?
bool str_is_eng (char* s);

///문자열이 영문 또는 한글인가?
int str_is_eng_kor (char* s);
