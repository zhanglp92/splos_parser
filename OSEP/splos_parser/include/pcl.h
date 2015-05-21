/*************************************************************************
	> File Name: ucl.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月13日 星期四 21时49分34秒

	> Description: 
 ************************************************************************/

#ifndef __UCL_H
#define __UCL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

#include "alloc.h"
#include "vector.h"
#include "type.h"
#include "str.h"
#include "input.h"
#include "error.h"
#include "lex.h"
#include "symbol.h"


/* 字节对齐 */
#define ALIGN(size, align) ((size + align - 1) & (~(align - 1)))
enum {false, true};


/* 当前可以分配的堆 */
extern Heap CurrentHeap;
/* 保存字符串的堆 */
extern struct heap StringHeap;
/* 用户自定义关键字空白符号 */
extern Vector ExtraWhiteSpace;
extern Vector ExtraKeywords;
/* 警告，错误数量 */
extern int WarningCount;
extern int ErrorCount;

/* 语法树输出文件 */
extern FILE *ASTFile;
extern FILE *ASMFile;
extern FILE *IRFile;

void UclTest (void);

#endif
