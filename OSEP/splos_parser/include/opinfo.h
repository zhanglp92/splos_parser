/*************************************************************************
	> File Name: opinfo.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月20日 星期四 23时15分51秒

	> Description: 
 ************************************************************************/

#ifndef OPINFO
#error "You must define OPINFO macro before include this file"
#endif

/* OPINFO (各种操作的类别码, 优先级, 操作的字符串表示, 操作的字节码) */

/* 操作符的信息 */
OPINFO(OP_COMMA,         1,    ",",      Comma,          NOP)
OPINFO(OP_ASSIGN,        2,    "=",      Assignment,     NOP)
OPINFO(OP_BITOR_ASSIGN,  2,    "|=",     Assignment,     NOP)
OPINFO(OP_BITXOR_ASSIGN, 2,    "^=",     Assignment,     NOP)
OPINFO(OP_BITAND_ASSIGN, 2,    "&=",     Assignment,     NOP)
OPINFO(OP_LSHIFT_ASSIGN, 2,    "<<=",    Assignment,     NOP)
OPINFO(OP_RSHIFT_ASSIGN, 2,    ">>=",    Assignment,     NOP)
OPINFO(OP_ADD_ASSIGN,    2,    "+=",     Assignment,     NOP)
OPINFO(OP_SUB_ASSIGN,    2,    "-=",     Assignment,     NOP)
OPINFO(OP_MUL_ASSIGN,    2,    "*=",     Assignment,     NOP)
OPINFO(OP_DIV_ASSIGN,    2,    "/=",     Assignment,     NOP)
OPINFO(OP_MOD_ASSIGN,    2,    "%=",     Assignment,     NOP)
OPINFO(OP_QUESTION,      3,    "?",      Conditional,    NOP)
OPINFO(OP_COLON,         3,    ":",      Error,          NOP)
/* 双目运算 */
OPINFO(OP_OR,            4,    "||",     Binary,         NOP)
OPINFO(OP_AND,           5,    "&&",     Binary,         NOP)
OPINFO(OP_BITOR,         6,    "|",      Binary,         BOR)
OPINFO(OP_BITXOR,        7,    "^",      Binary,         BXOR)
OPINFO(OP_BITAND,        8,    "&",      Binary,         BAND)

OPINFO(OP_EQUAL,         9,    "==",     Binary,         JE)
OPINFO(OP_UNEQUAL,       9,    "!=",     Binary,         JNE)
OPINFO(OP_GREAT,         10,   ">",      Binary,         JG)
OPINFO(OP_LESS,          10,   "<",      Binary,         JL)
OPINFO(OP_GREAT_EQ,      10,   ">=",     Binary,         JGE)
OPINFO(OP_LESS_EQ,       10,   "<=",     Binary,         JLE)

OPINFO(OP_LSHIFT,        11,   "<<",     Binary,         LSH)
OPINFO(OP_RSHIFT,        11,   ">>",     Binary,         RSH)
OPINFO(OP_ADD,           12,   "+",      Binary,         ADD)
OPINFO(OP_SUB,           12,   "-",      Binary,         SUB)
OPINFO(OP_MUL,           13,   "*",      Binary,         MUL)
OPINFO(OP_DIV,           13,   "/",      Binary,         DIV)
OPINFO(OP_MOD,           13,   "%",      Binary,         MOD)
/* 强制类型转换 */
OPINFO(OP_CAST,          14,   "cast",   Unary,          NOP)
/* 滞前自增减 */
OPINFO(OP_PREINC,        14,   "++",     Unary,          NOP)
OPINFO(OP_PREDEC,        14,   "--",     Unary,          NOP)
OPINFO(OP_ADDRESS,       14,   "&",      Unary,          ADDR)
OPINFO(OP_DEREF,         14,   "*",      Unary,          DEREF)
OPINFO(OP_POS,           14,   "+",      Unary,          NOP)
OPINFO(OP_NEG,           14,   "-",      Unary,          NEG)
OPINFO(OP_COMP,          14,   "~",      Unary,          BCOM)
OPINFO(OP_NOT,           14,   "!",      Unary,          NOP)
OPINFO(OP_SIZEOF,        14,   "sizeof", Unary,          NOP)
OPINFO(OP_INDEX,         15,   "[]",     Postfix,        NOP)
OPINFO(OP_CALL,          15,   "call",   Postfix,        NOP)
OPINFO(OP_MEMBER,        15,   ".",      Postfix,        NOP)
OPINFO(OP_PTR_MEMBER,    15,   "->",     Postfix,        NOP)
/* 滞后自增减 */
OPINFO(OP_POSTINC,       15,   "++",     Postfix,        INC)
OPINFO(OP_POSTDEC,       15,   "--",     Postfix,        DEC) 

OPINFO(OP_ID,            16,   "id",     Primary,        NOP)
OPINFO(OP_CONST,         16,   "const",  Primary,        NOP)
OPINFO(OP_STR,           16,   "str",    Primary,        NOP)
OPINFO(OP_NONE,          17,   "nop",    Error,          NOP)
