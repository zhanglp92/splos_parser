/*************************************************************************
	> File Name: lex.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 21时43分10秒

	> Description: 词法分析,定义token,和存储变量值的数据结构
 ************************************************************************/

#ifndef __LEX_H
#define __LEX_H

/* 枚举列出TOKEN 的序号 
 * 即，所有关键字，标示符，分隔符等的类别码 */
enum token {

    TK_BEGIN, 
    #define TOKEN(k, s) k, 
    #include "token.h"
    #undef TOKEN
};

/* 变量值的存储 */
union value {

    /* 整形值 */
    int i[2];
    /* float类型的值 */
    float f;
    /* double类型的值 */
    double d;
    /* 指针的值 */
    void *p;
};

/* 判断数字,字母等 */
static int IsDigit(int c)      {return ('0' <= c && '9' >= c);}
static int IsOctDigit(int c)   {return ('0' <= c && '7' >= c);} 
static int IsHexDigit(int c)   {return (IsDigit (c) || ('A' <= c \
            && 'F' >= c) || ('a' <= c && 'f' >= c));}
static int IsLetter(int c)     {return (('a' <= c && 'z' >= c) ||  \
            ('A' <= c && 'Z' >= c) || ('_' == c));}
static int IsLetterOrDigit(int c)  {return (IsLetter (c) || IsDigit (c));}
/* 所有的字母变成大写 */
#define ToUpper(c)          (c & ~0x20)

/* 取得一个int 的高4,3,1位 */
#define HIGH_4BIT(v)    ((v) >> (8 * INT_SIZE - 4) & 0x0f)
#define HIGH_3BIT(v)    ((v) >> (8 * INT_SIZE - 3) & 0x07)
#define HIGH_1BIT(v)    ((v) >> (8 * INT_SIZE - 1) & 0x01)


/* 初始化词法分析用到的数据 */
void SetupLexer (void);
void BeginPeekToken (void);
void EndPeekToken (void);
int GetNextToken (void);
void LexTest (char *filename);
void Import (void);

/* 当前单词的描述 */
extern union value  TokenValue;
extern struct coord TokenCoord;
/* 前一个单词的位置 */
extern struct coord PrevCoord;
extern char         *TokenString[];

extern Vector UserIncPath;

#endif
