//
//  Source: dtype.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-01-17 데이터 타입 헤더파일 정의
//		2011-01-18 BTREE 구조체에 중복키 허용여부 필드(samek)를 추가하다.
//		2011-02-11 함수 명칭에 규칙을 부여하다.(명칭 만들기 규칙)
//					함수명의 접두어는 소속 파일명이고, private 함수는 접두어 앞에 _를 붙인다.
//		2011-02-24 해시값 상수(HASH)를 추가하다.
//		2011-03-16 번용 문장 보관용 스택(StackTW)을 전역변수로 추가하다.
//		yyyy-mm-dd ...
//

#ifndef __DTYPE
#define __DTYPE

#define __LINUX
//#define __WIN32

//안정버전: V.yyyy.mm
//개발버전: V.yyyy.mm.dd
#define TWVersion "**TransWorks V1.2011.04"

#define ASIZE		  80	//단어 길이
#define SSIZE		1000	//문장 길이 (1000문자, 약100단어)

#define HashSizeStr	 "10"	//데이터 파일 헤더에 사용
#define HASHSIZE	  10	//(0 < 해시값 < HASHSIZE), 0은 캡션 문장용, 해시값을 감소시키면 이전 데이터를 읽지못함.
#define HASH		  31	//해시값 계산시 곱해지는 값(최적의 해시값 산출을 위해)

#define	LF	0x0A	//Line Feed
#define CR	0x0D	//Carige Return

//작업 구분값(flag), tw1.c에서 사용 -------------------------------------------
#define	FLAG_NONE	0
#define	FLAG_INSERT	1
#define	FLAG_DELETE	2
#define	FLAG_UPDATE	3
#define	FLAG_TRANS	4
#define	FLAG_VIEW	5
#define	FLAG_AUTO	6
#define	FLAG_CAP	7

#define	FLAG_YES	 1
#define	FLAG_NO		-1

//unsigned int 최대값(0의 보수)
#define UIFAIL		~0

//입출력 파일명 --------------------------------------------------------------
#define DIR_WORKS	"./works/"	//사용자 작업 경로

//아래 파일들은 TW 데이터 파일 이므로 현재경로(./)에서 작업됨
#define FNAME_DIC0	"twd0.twd"  //단어사전 A
#define FNAME_DIC1	"twd1.twd"  //단어사전 B

//매크로는 매개변수에 상수값이 전달되어야 함
//#define MR_FNAME(num) "twd" ## num ## ".twi"

#define FNAME_IDX	"twd"
#define FNAME_EXT	".twi"

#define FNAME_DAT0	"twd0.twa"  //데이터(교정단어 변환용)
#define FNAME_DAT1	"twd1.twa"  //데이터(삭제된 단어 고유번호 보관용 A)
#define FNAME_DAT2	"twd2.twa"  //데이터(삭제된 단어 고유번호 보관용 B)
#define FNAME_DAT3	"twd3.twa"	//도움말 파일
#define FNAME_DAT4	"twd4.twa"	//도움말 파일(회원)

#define FNAME_KEY	"twdk.twa"	//유료회원 개인키 파일

#define MANAGER		"J##)&J#"	//관리자
#define TRANS_CNT	     5		//비회원 문장번역 개수
#define PKEY_DDAY	   365		//개인키 유효일(1년)
#define SIMILAR			 5		//Similar 검색(full scan) 대상 단어수

#define STACK_HEIGHT   100		//번역문장 보관용 스택의 높이

//boolean 데이터 타입(gcc: #include <stdbool.h>) ------------------------------
typedef enum {
    true = 1,
    TRUE = 1,
    false = 0,
    FALSE = 0
} bool;

//자료구조(스택, 큐, 리스트) 구조체 선언 --------------------------------------

//단방향 노드(일반적인 스택, 큐, 리스트)
typedef struct node
{
	void*			data;
	struct node*	link;
} NODE;

//양방향 노드(진보된 스택-->리스트)
typedef struct node2
{
	struct node2*	prev;	//prev link
	void*			data;
	struct node2*	next;	//next link
} NODE2;

//스택 구조체
typedef struct
{
	NODE2*	top;
	NODE2*	bottom;
	int		count;
} STACK;

//큐 구조체
typedef struct
{
	NODE* front;
	NODE* rear;
	int count;
} QUEUE;


STACK*	StackTW;	//번역한 문장 저장용 스택(전역)
bool StackTW_Enable;


//B+Tree 구조체 정의 ------------------------------------------------------------------------------

//옥스포드 영어사전에 등재되어 있는 영단어 수는 약30만개(최대 100만개 예상)
//8의 6승      262144      ( 26만)
//8의 7승     2097152      (209만) *
//8의 8승    16777216     (1677만)
//8의 9승   134217728  (1억3421만) #
//8의10승  1073741824 (10억7374만)

//6의 8승  1,679,616
//6의 9승 10,077,696

//배열 오더 정의(트리 노드안의 엔트리 개수)
//B_ORDER가 짝수일때, 리프분할되는 개수가 일정해짐 (키순서로 순차입력시 필요한 조건)
#define B_ORDER	4
///#define B_ORDER		8	//일반단어
///#define R_ORDER		6	//교정단어
#define R_ORDER		4	//교정단어

//테스트된 데이터:
//단어수(key)가 약30만개 일때:
//(1) B+Tree 높이 (오더가 8일때 9, 오더가 6일때 11)
//(2) 데이터 파일 크기는 모두 약 4x3=12M
//(3) 메모리 할당 크기는 모두 약 240M

//메모리 할당: 단어수 10만개당 100M

//key는 unsigned int 데이터 타입, 최대 0xFFFFFFFF(32비트: 10자리: 4,294,967,295(약42억))개


//B+ 트리의 노드 정의:
//노드의 핵심은 keys의 배열이며 pointers와의 연관성이다.
//keys와 pointers의 관계는 leaves와 내부노드 사이에서 달라진다.
//리프에서 각각의 키 인덱스는 이것과 연관된 pointer 인덱스와 동일하며, 최대 (B_ORDER - 1)개의 키-포인터 쌍을 가진다.
//마지막 pointer는 오른쪽 리프를 가르킨다.
//내부노드에서는 첫번째 pointer는 키 배열에서 가장작은 키보다 작은 키로된 노드들을 가르키고,
//(i+1)번째 pointer는 인덱스 i번째 노드의 키보다 크거나 같은 키들을 가르킨다.
//num_keys 필드는 유효한 키들의 개수이다.
//내부노드에서 유효한 pointer들의 개수는 항상 (num_keys + 1)이다.
//리프에서는 데이터에 유효한 pointer들의 개수는 항상 num_keys 이다.
//마지막 리프 pointer는 다음 리프를 가르킨다.
typedef struct node5
{
	struct node5*	parent;		//부모노드 포인터
	bool			is_leaf;	//리프노드인가?
	int				num_keys;	//노드안의 키 개수

	void**			pointers;	//내부노드 및 레코드(데이터) 포인터 엔터리(배열)
	void**			keys;		//키값 엔터리(배열)

	struct node5*	next;		//노드를 출력할때 Queue Node로 사용(디버그용)
} NODE5;

//B+트리 시작 구조체
typedef struct
{
	NODE5*			root;
	int				order;		//B+트리 오더(엔터리 배열크기)
	unsigned int	kno;		//키고유번호(계속증가됨)
	unsigned int	kcnt;		//키개수(삽입증가, 삭제감소)
	int  (*compare) (void* p1, void* p2);	//키비교 함수포인터
	void (*outkey)  (void* p1);				//키출력 함수포인터
	void (*outdata) (void* p1);				//데이터출력 함수포인터
	bool			samek;		//중복키 허용여부
} BTREE;

#endif
