//
//  Source: main.c written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-01-01 main 함수를 독립시키다.
//		2011-01-28 숫자조합 인데스키를 2개로 분리하다.
//		2011-02-08 교정단어 변환용 B+트리(btree_rs)를 추가하다.
//		2011-02-11 단어 삭제시 고유번호 관리용 큐(QUEUE* qk[])를 추가하다.
//		2011-02-25 번역키 B+트리를 HASHSIZE 만큼 생성하여 해시값으로 선택하도록 하다.(용량및 속도향상)
//		yyyy-mm-dd ...
//

#include <stdio.h>
#include <stdlib.h>

#include "dtype.h"		//데이터 타입
#include "bpt3.h"		//B+트리
#include "ustr.h"		//문자열
#include "utime.h"		//시간
#include "fileio.h"		//파일 입출력
///#include "queue.h"		//큐
///#include "stack.h"		//스택
#include "tw1.h"

//메인 함수 ---------------------------------------------------------------------------------------
int main (int argc, char** argv)
{
	BTREE	*btree;
	register int i;
	int		err = 0;
	double	msec1, msec2;

	printf ("btree test running...\n");

	//미리 초 단위(시작)
	msec1 = time_get_msec ();

	//B_ORDER; //default 8
	//B_ORDER는 B+트리 노드에서 entries(keys and pointers)의 최대 및 최소 개수를 결정한다.

	//단어키 B+트리 A 생성
	btree = bpt_create (B_ORDER, tw1_compare_str, tw1_output_str, tw1_output_int, false);	//중복키 허용않함
	if (!btree) err++;

	if (err) {
		printf ("## Memory allocation error(%) at the bpt_create().\n", err);
		exit (EXIT_FAILURE);
	}

    ///test block
    {
        NODE5	*leaf, *root;
        char	*pword, s[6];
        unsigned int *pno;

        for (i=0; i < 20; i++) {
            bpt_find_leaf_key (btree, pno, &leaf);

            pword = malloc (6);
            str_copy (pword, "dat");
            uint_to_str (i, s);
            strcat(pword, s);
            pno = malloc (sizeof(unsigned int));
            *pno = i;
            ///순자키 입력
            btree->root = bpt_insert (btree, leaf, pno, pword, FLAG_INSERT);
            root = btree->root;
        }
    }
    //미리 초 단위(종료)
    msec2 = time_get_msec ();
    //실행시간
    printf ("** Run Time: %.3f Secs\n", msec2 - msec1);

    printf ("btree test finished.\n\n");

	return EXIT_SUCCESS;
}
