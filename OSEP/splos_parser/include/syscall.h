/*************************************************************************
	> File Name: syscall.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年01月05日 星期一 17时04分42秒

	> Description: 系统调用
 ************************************************************************/

#ifndef     SYSCALL
#error "You must define SYSCALL macro before include this file"
#endif


/* SYSCALL (系统调用的类别, 系统调用的名称, 名称长度, 系统调用号) */
/* 系统调用0 号表示普通调用 */

SYSCALL (SYS_OPEN,      "open",     4, 1)
SYSCALL (SYS_CLOSE,     "close",    5, 2)
