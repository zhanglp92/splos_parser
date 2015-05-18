/*************************************************************************
	> File Name: stmt.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月29日 星期六 18时27分51秒

	> Description: 语句语法分析的头文件
 ************************************************************************/

#ifndef __STMT_H
#define __STMT_H


#define AST_STATEMENT_COMMON AST_NODE_COMMON

/* 循环公共属性 
 * expr :   循环的条件
 * stmt :   循环体 
 * loopBB:  循环体基本块
 * contBB:  循环条件基本块 
 * nextBB:  循环结束之后的基本块 */
#define AST_LOOP_STATEMENT_COMMON   \
    AST_STATEMENT_COMMON;           \
    AstExpression expr;             \
    AstStatement stmt;              \
    BBlock loopBB, contBB, nextBB

typedef struct astStatement {

    AST_STATEMENT_COMMON;
} *AstStatement;

/* 复合语句的描述 */
typedef struct astCompoundStatement {

    AST_STATEMENT_COMMON;
    /* 变量定义链表 */
    AstNode decls;
    /* 语句链表 */
    AstNode stmts;
    /* 局部变量集合 */
    Vector ilocals;
} *AstCompoundStatement;

/* 描述标签 */
typedef struct astLabelStatement {
    
    AST_STATEMENT_COMMON;
    /* 标签名字 */
    char *id;
    /* 标签后边的语句列表 */
    AstStatement stmt;
    Label label;
} *AstLabelStatement;

/* 表达式语句 */
typedef struct astExpressionStatement {
    
    AST_STATEMENT_COMMON;
    /* 表达式 */
    AstExpression expr;
} *AstExpressionStatement;

/* 描述case 模块 */
typedef struct astCaseStatement {
    
    AST_STATEMENT_COMMON;
    /* case 后的表达式 */
    AstExpression expr;
    /* case 语句 */
    AstStatement stmt;
    /* 链接下一个case */
    struct astCaseStatement *nextCase;
    /* case的基本块 */
    BBlock respBB;
} *AstCaseStatement;

/* 描述default 模块 */
typedef struct astDefaultStatement {
    
    AST_STATEMENT_COMMON;
    /* default 语句 */
    AstStatement stmt;
    BBlock respBB;
} *AstDefaultStatement;

/* 描述并行语句 */
typedef struct astParallelStatement {
    
    AST_STATEMENT_COMMON;
    /* 并行语句模块 */
    AstStatement stmt;
    BBlock  respBB;
} *AstParallelStatement;

/* 描述if 模块 */
typedef struct astIfStatement {
    
    AST_STATEMENT_COMMON;
    /* if 条件 */
    AstExpression expr;
    /* if then 语句 */
    AstStatement thenStmt;
    /* else 模块 */
    AstStatement elseStmt;
} *AstIfStatement;

/* 描述switch 内部的结构属性值 */
typedef struct switchBucket {

    /* case 的个数 */
    int ncase;
    int minVal;
    int maxVal;
    /* case 语句链表 */
    AstCaseStatement cases;
    AstCaseStatement *tail;
    struct switchBucket *prev;
} *SwitchBucket;

/* 描述switch 模块 */
typedef struct astSwitchStatement {
    
    AST_STATEMENT_COMMON;
    /* switch 表达式 */
    AstExpression expr;
    /* switch 里的语句 */
    AstStatement stmt;
    /* case 语句模块 */
    AstCaseStatement cases;
    /* default 语句模块 */
    AstDefaultStatement defStmt;
    SwitchBucket buckets;

    int nbucket;
    BBlock nextBB;
    BBlock defBB;
} *AstSwitchStatement;

/* 描述循环 */
typedef struct astLoopStatement {
    
    AST_LOOP_STATEMENT_COMMON;
} *AstLoopStatement;

/* 描述for 模块 */
typedef struct astForStatement {
    
    AST_LOOP_STATEMENT_COMMON;
    /* for 的初始化和步长表达式 */
    AstExpression initExpr, incrExpr;
    BBlock testBB;
} *AstForStatement;

/* 描述goto 操作 */
typedef struct astGotoStatement
{
	AST_STATEMENT_COMMON;
	char *id;
    /* 要跳转到的标签处 */
	Label label;
} *AstGotoStatement;

/* 描述break 操作 */
typedef struct astBreakStatement
{
    /* 中的kind 表示是switch,或loop 的跳转 */
	AST_STATEMENT_COMMON;
    /* 跳出的目标 */
	AstStatement target;
} *AstBreakStatement;

/* 描述continue 操作 */
typedef struct astContinueStatement
{   
	AST_STATEMENT_COMMON;
    /* 跳到的目标出 */
	AstLoopStatement target;
} *AstContinueStatement;

/* 描述return 操作 */
typedef struct astReturnStatement 
{
	AST_STATEMENT_COMMON;
    /* 返回值 */
	AstExpression expr;
} *AstReturnStatement;

/* 强制类型转换的简写 */
#define AsExpr(stmt)   ((AstExpressionStatement)stmt)
#define AsLabel(stmt)  ((AstLabelStatement)stmt)
#define AsCase(stmt)   ((AstCaseStatement)stmt)
#define AsDef(stmt)    ((AstDefaultStatement)stmt)
#define AsIf(stmt)     ((AstIfStatement)stmt)
#define AsSwitch(stmt) ((AstSwitchStatement)stmt)
#define AsLoop(stmt)   ((AstLoopStatement)stmt)
#define AsFor(stmt)    ((AstForStatement)stmt)
#define AsGoto(stmt)   ((AstGotoStatement)stmt)
#define AsCont(stmt)   ((AstContinueStatement)stmt)
#define AsBreak(stmt)  ((AstBreakStatement)stmt)
#define AsRet(stmt)    ((AstReturnStatement)stmt)
#define AsComp(stmt)   ((AstCompoundStatement)stmt)
#define AsParall(stmt) ((AstParallelStatement)stmt)

#if 1
AstStatement CheckCompoundStatement(AstStatement stmt);
#endif


#endif
