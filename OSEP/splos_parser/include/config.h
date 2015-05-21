/*************************************************************************
	> File Name: config.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 13时38分21秒

	> Description: 类型的属性值
 ************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

#define CHAR_SIZE           1
#define SHORT_SIZE          2
#define INT_SIZE            4
#define LONG_SIZE           4
#define LONG_LONG_SIZE      8
#define FLOAT_SIZE          4
#define DOUBLE_SIZE         8
#define LONG_DOUBLE_SIZE    16

/* 定义宽字符的序号 */
#ifdef _WIN32 
    #define WCHAR       USHORT 
    #define WCHAR_SIZE  SHORT_SIZE
#else
    #define WCHAR       ULONG
    #define WCHAR_SIZE  LONG_SIZE
#endif

#define _LF

#endif
