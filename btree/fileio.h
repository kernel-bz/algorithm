//
//  Source: fileio.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-12-21 파일 입출력 함수들을 작성하다.
//		yyyy-mm-dd ...
//

#include "dtype.h"	//B+ Tree 헤더

//파일에서 단어를 읽어서 B+트리에 할당후 그 개수를 반환
int _fio_read_header (FILE *fp, char *fname, char *buf);  //파일 헤더 읽기
unsigned int fio_read_from_file (char *fname, BTREE* btree);
unsigned int fio_read_trans_asc (char *fname, BTREE* btree, int flag);
unsigned int fio_read_trans_hash (char *fname, BTREE** hb[], int sh);

//B+트리의 내용을 파일에 저장
void _fio_write_header (FILE *fp, char *fname);  //파일 헤더 쓰기
unsigned int fio_write_to_file (char *fname, BTREE* btree);
unsigned int fio_write_to_file_trans (char *fname, BTREE* btree);

///삭제된 단어의 고유번호를 파일에서 읽어서 queue에 저장
unsigned int fio_read_from_file_kno (char *fname, QUEUE* queue);
///queue에 있는 삭제된 단어의 고유번호를 파일에 저장
unsigned int fio_write_to_file_kno (char *fname, QUEUE* queue);

///번역 데이터 파일(txt)에서 번역행들을 읽어서 저장(입력)
int fio_import (char *fname, BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk);
///번역정보를 파일에 출력
int _fio_export_data (FILE *fp, BTREE* wi, void* keys);
///번역정보를 버퍼에 저장
int _fio_export_buffer (char sbuf[], BTREE* wi, void* keys);
///키를 하나씩 분리하여 단어 단위로 출력
int _fio_trans_key_each (FILE *fp, char* keys, BTREE** hb[], BTREE** wi, int mode, int kcnt);
///번역 데이터을 파일에 출력(저장)
void fio_export (char* fname, BTREE* pt, BTREE* pi_src, BTREE* pi_trg);
///교정(revision) 단어를 파일(txt)에서 입력(읽기)
int fio_import_revision (char *fname, BTREE* rs);
///교정(revision) 단어를 파일(txt)에 출력(저장)
void fio_export_revision (char* fname, BTREE* rs);

//파일에 있는 문장 번역(mode=0는 영문번역, 1은 한글번역)
int fio_translation (char *fname, char *fname2, BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk, int flag);

//도움말 파일을 읽어서 출력
bool fio_read_help (char *fname);

//경로 및 파일명 관련 함수
int fio_mkdir (char *dir);
int fio_getchar_fname (char* msg, char* fname);

//유료회원 개인키 저장
int fio_write_member_key (char* fname, char* skey);
//유료회원 개인키 읽기
int fio_read_member_key (char* fname, char* skey);

///번역 데이터 파일(txt)에서 CAPTION 문장들을 읽어서 저장(입력)
int fio_import_caption (char *fname, BTREE** ws, BTREE** wi, BTREE** hb[], int mode, BTREE* rs, QUEUE** qk);
