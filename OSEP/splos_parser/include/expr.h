/*************************************************************************
	> File Name: expr.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月19日 星期三 21时10分59秒

	> Description: 分析表达式的语法
 ************************************************************************/

#ifndef __EXPR_H
#define __EXPR_H

/* 枚举各种操作 */
enum OP {

    #define OPINFO(op, prec, name, func, opcode) op, 
    #include "opinfo.h"
    #undef  OPINFO
};

/* 操作 */
struct tokenOp {

    /* 二元操作 */
    int bop :16;
    /* 一元操作 */
    int uop :16;
};

/* 描述一个表达式节点 */
typedef struct astExpression {

    AST_NODE_COMMON;
    /* 表达式的类型 */
    Type ty;
    /* 表达式操作符 */
    int op  :16;
    /* 是否为数组 */
    int isarray :1;
    /* 是否为函数 */
    int isfunc  :1;
    /* 是否为左值 */
    int lvalue  :1;
    /* 是否为位段 */
    int bitfld  :1;
    /* 是否被声明为寄存器变量 */
    int inreg   :1;
    /* 没有被使用 */
    int unused  :11;
    /* 函数调用号(普通函数0, 系统调用则是系统调用号) */
    unsigned callNum;
    /* 表达式的操作数 */
    struct astExpression *kids[2];
    /* 表达式的值 */
    union value val;
} *AstExpression;

/* 判断是否为二元操作符 */
#define IsBinaryOP(tok)  (tok >= TK_OR && tok <= TK_MOD)
/* 双目操作符 */
#define BINARY_OP       TokenOps[CurrentToken - TK_ASSIGN].bop
/* 单目操作符 */
#define UNARY_OP        TokenOps[CurrentToken - TK_ASSIGN].uop

//AstExpression ParseConstantExpression (void);


/* 调整类型 */
AstExpression Adjust (AstExpression expr, int rvalue);
/* 表达式检查 */
AstExpression CheckExpression (AstExpression expr);
/* 判断初始化的数据能否正确分配相应的字段 */
int CanAssign (Type lty, AstExpression expr);
/*  */
AstExpression Cast (Type ty, AstExpression expr);
/* 构造一个常量表达式 */
AstExpression Constant (struct coord coord, Type ty, union value val);
/* Cast constant expression. e.g. (float)3 */
AstExpression FoldCast (Type ty, AstExpression expr);
/* Constant folding. e.g. 3 + 4 */
AstExpression FoldConstant (AstExpression expr);
/* 构造常量0 */
void StructCosntant0 (void);

/* 翻译表达式 */
Symbol TranslateExpression (AstExpression expr);
/* 翻译分支语句 */
void TranslateBranch (AstExpression expr, BBlock trueBB, BBlock falseBB);
/* 返回非表达式 */
AstExpression Not (AstExpression expr);


extern AstExpression Constant0;

#endif
