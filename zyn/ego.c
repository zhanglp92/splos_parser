/*************************************************************************
	> File Name: ego.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年02月07日 星期六 17时38分05秒

	> Description: 
 ************************************************************************/

#include "ego.h"


/* 扫描文件,生成对应的概率 */
static void Compile (const char *file)
{
    Scanner (file);
}

int main (int argc, char **argv)
{
    int     i = 1;

    SetupEgo ();

    for (i; i < argc; i++) {

        /* 扫描文件 */
        Compile (argv[i]);
    }
    
    SimpSort ();
    displayProbab ();


    return 0;
}
