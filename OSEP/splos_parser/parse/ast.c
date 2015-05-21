/*************************************************************************
	> File Name: ast.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月20日 星期四 16时07分46秒

	> Description: 语法分析的公用函数
 ************************************************************************/

#include "pcl.h"
#include "ast.h"

/* 表示当前词法单元的类型 */
int CurrentToken;

/* 当前期望的词法单元 */
void Expect (int tok)
{
    /* 是当前期望的token */
    if (CurrentToken == tok) {

        NEXT_TOKEN;
        return ;
    }
    
    Error (&TokenCoord, "Expect %s", TokenString[tok]);
}

/* 在一个tok集合中查找当前需要的tok */
int CurrentTokenIn (int toks[]) 
{
    int *p = toks;

    for ( ; *p; p++) 
        if (CurrentToken == *p) 
            return 1;

    return 0;
}

void SkipTo (int toks[], const char *einfo) 
{
    int *p = toks;
    struct coord cord;

    /* 找到了当前tok 或者扫描结束 */
    if (CurrentTokenIn (toks) || TK_END == CurrentToken) 
        return ;

    cord = TokenCoord;
    while (TK_END != CurrentToken) {

        for (p = toks; *p; p++) {

            if (CurrentToken == *p) 
                goto sync;
        }
        NEXT_TOKEN;
    }
sync:
    Error (&cord, "skip to %s\n", einfo);
}
