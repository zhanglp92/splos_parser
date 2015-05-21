/*************************************************************************
	> File Name: error.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 19时59分43秒

	> Description: 
 ************************************************************************/

#include "ego.h"

/* 致命的错误 */
void Fatal (const char *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    fprintf (stderr, "\n");
    va_end (ap);
    /* 遇见此类错误直接退出 */
    exit (-1);
}
