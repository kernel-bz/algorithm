//
//  Source: utime.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2010-12-18 시간 관련 함수들을 작성하다.
//		yyyy-mm-dd ...
//

void time_rand_seed_init (void);

//p와 r사이의 난수 발생
unsigned int time_random_between (unsigned int p, unsigned int r);

//무작위 문자열(영문 소문자)을 cnt 개수 만큼 생성
char* time_get_random_str (int cnt);


//초 단위로 현재시간을 가져옴
long time_get_sec (void);

//미리초 단위로 현재시간을 가져옴
double time_get_msec (void);

void time_sleep (int sec);
