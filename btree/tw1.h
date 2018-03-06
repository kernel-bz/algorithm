//
//  Source: tw1.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-01-01 tw(TransWorks) 헤더파일로 독립시키다.
//		2011-02-09 교정단어 보정기능 추가하다.
//		yyyy-mm-dd ...
//

#include "dtype.h"

//B+Tree 내부 함수
int tw1_compare_int (void* p1, void* p2);
int tw1_compare_str (void* p1, void* p2);
//숫자키 문자열을 unsigned int로 변환후 비교
int tw1_compare_str_int (void* p1, void* p2);
void tw1_output_int (void* p1);
void tw1_output_str (void* p1);

int tw1_qn_answer (char *msg, int ans);

//입력
unsigned int _tw1_insert_to_btree (BTREE*, BTREE*, char *in_word, QUEUE*);
bool _tw1_insert_to_btree_trans (BTREE*, BTREE*, char* keys1, char* keys2);
int tw1_insertion (BTREE**, BTREE**, BTREE** hb[], int mode, BTREE* rs, QUEUE**);
int tw1_insertion_from_file (BTREE**, BTREE**, BTREE** hb[], int, BTREE*, QUEUE**, char rows[2][SSIZE], int sh);

//번역 출력
int  _tw1_trans_key_data (BTREE*, void* keys, int kcnt);
int  _tw1_trans_key_data_each (BTREE* wi, void* keys);
int  _tw1_trans_key_finder (NODE5* leaf, int k, char *keys, BTREE* pt1, BTREE* hb[], BTREE* pi_trg, int flag);
void _tw1_trans_key_equal (NODE5* leaf, char *keys, int k, BTREE* pt1, BTREE* pi_trg);
int  _tw1_trans_key_like (char *keys, BTREE* pt, BTREE* pi_src, BTREE* pi_trg, int kcnt);
int  _tw1_trans_key_similar (char *keys, BTREE* pt, BTREE* pi_src, BTREE* pi_trg);
int  _tw1_trans_key_each (char* keys, BTREE** hb[], BTREE** wi, int mode, int kcnt);
int  _tw1_trans_search (char* keys, BTREE** hb[], BTREE** wi, BTREE* rs, int mode, int kcnt);
void tw1_translation (BTREE**, BTREE** hb[], BTREE**, int mode, char* prompt, BTREE* rs, QUEUE**);

//삭제
unsigned int tw1_drop_word_run (BTREE*, BTREE*, char *key, bool *deleted);
bool tw1_drop_word (BTREE* ws, BTREE* wi, QUEUE* queue);
int _tw1_delete_trans_key_run (BTREE* pt1, BTREE* pt2, NODE5* leaf, int k);
int _tw1_delete_trans_key (char* keys, int cnt, BTREE** hb[], BTREE** wi, int mode);
int tw1_deletion (BTREE**, BTREE** hb[], BTREE**, int mode, char* prompt, BTREE* rs, QUEUE**);

//출력
void tw1_display (BTREE* hb[], BTREE*, BTREE*);

//테스트
//난수(정수와 문자열 최대 10자리)를 발생하여 삽입과 삭제를 반복실행(loop_max: 반복회수)
//키가 70% 정도 삽입되고, 30%는 삭제됨.
//반복을 100만번 수행시 약 100M 바이트의 메모리 소비. (개당 약100바이트)
int tw1_test_insert (BTREE* ws, BTREE* wi, char* words, char akeys[], QUEUE*);
///단어 입력 테스트
void _tw1_test_ins_word_random (BTREE* ws, BTREE* wi, unsigned int cnt_loop, QUEUE*);
///단어 삭제 테스트
void _tw1_test_del_word_random (BTREE* ws, BTREE* wi, unsigned int cnt_loop, QUEUE*);
void tw1_test_random (BTREE* ws[], BTREE* wi[], BTREE** hb[], unsigned int cnt_loop, QUEUE* qk[]);

//저장
void tw1_save (BTREE* ai, BTREE* bi, BTREE* hbta[], BTREE* rs);

//메뉴
char* _tw1_prompt (int mode);
char* _tw1_menu (int mode, int isave);
void tw1_menu_run (BTREE* ws[], BTREE* wi[], BTREE** hb[], BTREE*, QUEUE* qk[], int isave);
//관리자 메뉴
char* _tw1_manager_menu (int mode);
int tw1_manager (BTREE* ws[], BTREE* wi[], BTREE** hb[], int mode, BTREE*, QUEUE* qk[]);
//회원 메뉴(유료)
char* _tw1_member_menu (int mode);
int tw1_member (BTREE* ws[], BTREE* wi[], BTREE** hb[], int mode, BTREE* rs, QUEUE* qk[]);

//통계
void tw1_statis (BTREE* ws[], BTREE* wi[], BTREE** hb[], BTREE* rs, QUEUE** qk);

//단어 보정(교정단어 교정)
char* tw1_revision (BTREE* rs, char* skey);

//단어를 words 배열에 입력 받음
int _tw1_words_getchar (char* prompt, char words[], BTREE* ws);
//문자열을 입력받아 단어 단위의 인덱스 조합을 keys 배열에 저장
int tw1_words_input (char* prompt, char keys[], BTREE*, BTREE*, BTREE* rs, QUEUE*, int flag);

