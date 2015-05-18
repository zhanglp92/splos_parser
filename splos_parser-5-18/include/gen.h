/*************************************************************************
	> File Name: gen.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月23日 星期二 22时52分54秒

	> Description: 中间代码生成
 ************************************************************************/

#ifndef __GEN_H
#define __GEN_H

enum OPCode {

    #define OPCODE(code, name, func) code, 
    #include "opcode.h" 
    #undef OPCODE
};

/* 中间代码的指令定义 */
typedef struct irinst {

    /* 指向上一条指令 */
    struct irinst *prev;
    /* 指向下一条指令 */
    struct irinst *next;
    /* 指令操作类型 */
    Type ty;
    /* 操作码 */
    int opcode;
    union {
        
        /* 函数调用存储调用号 */
        int callNum;
    };
    /* 三地址码 */
    Symbol opds[3];
} *IRInst;


/* 表示前驱后继的链表 */
typedef struct cfgedge {

    BBlock bb;
    struct cfgedge *next;

} *CFGEdge;

/* 基本块 */
struct bblock {

    /* 指向上一个基本块 */
    struct bblock *prev;
    /* 指向下一个基本块 */
    struct bblock *next;
    /* 代表基本块的符号 */
    Symbol  sym;
    /* 该基本块所有的后继 */
    CFGEdge succs;
    /* 该基本块所有的前驱 */
    CFGEdge preds;
    /* 该基本块的指令列表 */
    struct irinst insth;
    /* 基本块中的指令数目 */
    int ninst;
    /* 后继个数 */
    int nsucc;
    /* 前驱个数 */
    int npred;
    /* 块的引用次数 */
    int ref;
    int no;
    /* 标志是否是并行块 */
    int isParall;
};

typedef struct ilarg {

    Symbol  sym;
    Type    ty;
} *ILArg;

BBlock  CreateBBlock (void);
void    StartBBlock (BBlock bb);
/* 中间代码优化 */
void    Optimize (FunctionSymbol fsym);
/* 插入一个块 */
void    DrawCFGEdge (BBlock head, BBlock tail);
/* 填充内存块 */
void    GenerateBranch (Type ty, BBlock dstBB, int opcode, Symbol src1, Symbol src2);
/* 构造一条指令, 并添加到当前块中 */
void    GenerateAssign (Type ty, Symbol dst, int opcode, Symbol src1, Symbol src2);
/* 生成间接跳转指令 */
void    GenerateIndirectJump (BBlock *dstBBs, int len, Symbol index);
/* 生成直接跳转指令 */
void    GenerateJump (BBlock dstBB);
/* 生成return 指令 */
void    GenerateReturn (Type ty, Symbol src);
void    GenerateClear (Symbol dst, int size);
/* 生成赋值指令 */
void    GenerateMove(Type ty, Symbol dst, Symbol src);
/* 添加临时变量 */
Symbol  TryAddValue (Type ty, int op, Symbol src1, Symbol src2);
/* 算数优化 */
Symbol  Simplify (Type ty, int op, Symbol src1, Symbol src2);
/* 间接赋值(解址后在赋值): a = *b */
void    GenerateIndirectMove (Type ty, Symbol dst, Symbol src);
/* 取址运算 */
Symbol  Deref (Type ty, Symbol addr);
Symbol  AddressOf (Symbol p);
/* 函数调用 */
void    GenerateFunctionCall (Type ty, Symbol recv, Symbol faddr, Vector args, int callNum);
/* 定义临时变量 */
void    DefineTemp (Symbol t, int op, Symbol src1, Symbol src2);


void    ExamineJump(BBlock bb);
BBlock  TryMergeBBlock(BBlock bb1, BBlock bb2);


/* 当前基本块 */
extern BBlock CurrentBB;
extern int OPMap[];
extern int isParall;

#endif
