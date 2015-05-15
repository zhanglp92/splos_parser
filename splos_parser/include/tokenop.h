/*************************************************************************
	> File Name: tokenop.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月20日 星期四 22时02分41秒

	> Description: 操作符的toke列表
 ************************************************************************/

#ifndef TOKENOP
#error "You must define TOKENOP macro before include this file"
#endif

/* TOKENOP (操作符类别码, 二元操作, 一元操作 ) */

/* 一元和二元操作 */
TOKENOP(TK_ASSIGN,        OP_ASSIGN,        OP_NONE) /* = */
TOKENOP(TK_BITOR_ASSIGN,  OP_BITOR_ASSIGN,  OP_NONE) /* |= */
TOKENOP(TK_BITXOR_ASSIGN, OP_BITXOR_ASSIGN, OP_NONE) /* ^= */
TOKENOP(TK_BITAND_ASSIGN, OP_BITAND_ASSIGN, OP_NONE) /* &= */
TOKENOP(TK_LSHIFT_ASSIGN, OP_LSHIFT_ASSIGN, OP_NONE) /* <<= */
TOKENOP(TK_RSHIFT_ASSIGN, OP_RSHIFT_ASSIGN, OP_NONE) /* >>= */
TOKENOP(TK_ADD_ASSIGN,    OP_ADD_ASSIGN,    OP_NONE) /* += */
TOKENOP(TK_SUB_ASSIGN,    OP_SUB_ASSIGN,    OP_NONE) /* -= */
TOKENOP(TK_MUL_ASSIGN,    OP_MUL_ASSIGN,    OP_NONE) /* *= */
TOKENOP(TK_DIV_ASSIGN,    OP_DIV_ASSIGN,    OP_NONE) /* /= */
TOKENOP(TK_MOD_ASSIGN,    OP_MOD_ASSIGN,    OP_NONE) /* %= */

/* || 到 % 被定义为二元操作 */
TOKENOP(TK_OR,            OP_OR,            OP_NONE) /* || */
TOKENOP(TK_AND,           OP_AND,           OP_NONE) /* && */
TOKENOP(TK_BITOR,         OP_BITOR,         OP_NONE) /* | */
TOKENOP(TK_BITXOR,        OP_BITXOR,        OP_NONE) /* ^ */
TOKENOP(TK_BITAND,        OP_BITAND,        OP_ADDRESS) /* & */
TOKENOP(TK_EQUAL,         OP_EQUAL,         OP_NONE) /* == */
TOKENOP(TK_UNEQUAL,       OP_UNEQUAL,       OP_NONE) /* != */
TOKENOP(TK_GREAT,         OP_GREAT,         OP_NONE) /* > */
TOKENOP(TK_LESS,          OP_LESS,          OP_NONE) /* < */
TOKENOP(TK_LESS_EQ,       OP_GREAT_EQ,      OP_NONE) /* >= */
TOKENOP(TK_GREAT_EQ,      OP_LESS_EQ,       OP_NONE) /* <= */
TOKENOP(TK_LSHIFT,        OP_LSHIFT,        OP_NONE) /* << */
TOKENOP(TK_RSHIFT,        OP_RSHIFT,        OP_NONE) /* >> */
TOKENOP(TK_ADD,           OP_ADD,           OP_POS)  /* + */
TOKENOP(TK_SUB,           OP_SUB,           OP_NEG)  /* - */
TOKENOP(TK_MUL,           OP_MUL,           OP_DEREF)/* * */
TOKENOP(TK_DIV,           OP_DIV,           OP_NONE) /* / */
TOKENOP(TK_MOD,           OP_MOD,           OP_NONE) /* % */

TOKENOP(TK_INC,           OP_NONE,          OP_PREINC) /* ++ */
TOKENOP(TK_DEC,           OP_NONE,          OP_PREDEC) /* -- */
TOKENOP(TK_NOT,           OP_NONE,          OP_NOT)  /* ! */
TOKENOP(TK_COMP,          OP_NONE,          OP_COMP) /* ~ */
