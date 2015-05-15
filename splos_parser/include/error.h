/*************************************************************************
	> File Name: error.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 20时17分22秒

	> Description:  错误警告等处理
 ************************************************************************/

#ifndef __ERROR_H
#define __ERROR_H

void Error (Coord coord, const char *fmt, ...);
void Fatal (const char *fmt, ...);
void Warning (Coord coord, const char *fmt, ...);

#endif
