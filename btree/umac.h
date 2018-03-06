//
//  Source: umac.h written by Jung,JaeJoon at the www.kernel.bz
//  Compiler: Standard C
//  Copyright(C): 2010, Jung,JaeJoon(rgbi3307@nate.com)
//
//  Summary:
//		2011-03-11 매크로 함수들을 작성하다. (매크로 명칭()는 서로 붙인다)
//		yyyy-mm-dd ...
//

//#include <stdio.h>
//#define um_dprint (param)  printf (#param " = %d\n", param)

#define um_max(a, b)  (a > b) ? a : b
#define um_min(a, b)  (a < b) ? a : b

#define um_square(x)  x * x

#define um_paste(p1, p2)  p1 ## p2

//문장 끝인가? (. ? !)
#define um_end(c)  (c==46 || c==63 || c==33) ? 1 : 0

//화이트스페이스 인가?
#define um_whites(c)  (c > 0 && c < 33) ? 1 : 0

//숫자인가?
#define um_digit(c)  (c > 47 && c < 58) ? 1 : 0
