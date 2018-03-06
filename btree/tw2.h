//
//  Source: tw2.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2011, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-02-11 tw2.h 모듈을 코딩하다.
//		yyyy-mm-dd ...
//

#include "dtype.h"

///교정(revision) 단어를 B+트리에 입력
bool tw2_rev_word_insert (BTREE* rs, char akey[][ASIZE]);
///교정(revision) 단어 키보에서 입력
int tw2_rev_word_input (BTREE* rs);
///교정단어 B+트리에서 삭제(단어단위)
int tw2_rev_word_delete (BTREE* rs);
///교정(revision) 단어 입력(파일)
int tw2_rev_word_import (BTREE* rs);
///교정(revision) 단어 출력(파일)
void tw2_rev_word_export (BTREE* rs);

///수정 실행(입력)
unsigned int _tw2_word_update_run (BTREE* ws, BTREE* wi, char *in_word, unsigned int kno);
///단어 수정(삭제후 입력)
int tw2_word_update (BTREE* ws, BTREE* wi, char* prompt);

///파일에서 번역된 문장들 입력
int tw2_import_from_file (BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, int flag);
///문장들을 파일에 출력(k=0: 단어, k=2: 문장)
void tw2_export_to_file (BTREE** hb[], BTREE** wi, int mode, int sh);

///파일안의 문장번역
void tw2_file_translation (BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, int flag);

///해시값을 하나 입력받음
int tw2_getchar_hash_value (void);
//관리자 비밀번호를 입력받음
bool tw2_getchar_manager_pw (void);

//개인키 암호화/복호화
int _tw2_getchar_encode_key (char* msg, char* akey, int flag);
void _tw2_get_decode_key (char* skey, char* out_key, char* out_key_time, int cnt, int flag);
int tw2_member_key_input (char *fname, int flag);
void tw2_member_key_output (char *fname);
int tw2_getchar_member_pw (char *fname);

//CAPTION 을 rs 트리에 저장
int tw2_insertion_caption (BTREE** ws, BTREE** wi, int mode, BTREE* rs, QUEUE** qk, char rows[][ASIZE]);

//번역한 문장 복습
void tw2_stack_review (void);
int tw2_stack_push (char* str);
void tw2_stack_pop (void);
