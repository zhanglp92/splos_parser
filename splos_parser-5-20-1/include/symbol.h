/*************************************************************************
	> File Name: symbol.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月19日 星期三 19时58分45秒

	> Description: 符号表管理
 ************************************************************************/

#ifndef __SYMBOL_H
#define __SYMBOL_H

/* 不同符号的类别 */
enum {
    SK_Tag, SK_TypedefName, SK_EnumConstant, SK_Constant, SK_Variable,
    SK_Temp, SK_Offset, SK_String, SK_Label, SK_Function, SK_Register
};

/* 符号表哈希值掩码 */
#define SYM_HASH_MASK   127

/* 符号的公有属性 
 * kind:    符号的类别
 * name:    符号的名字
 * aname:
 * ty:      符号的类型
 * level:   符号所在的级别(范围)
 * sclass:  符号的类别码(lex.h中定义)
 * ref:     符号被引用的次数
 * defined: 是否已经定义(若是函数,则表示是否有函数实现)
 * addressed:   是否是地址
 * needwb:
 * unvalid:   表示该符号是否有效(true 无效)
 * unused:
 * val:     符号的值
 * reg:     引用的次数
 * link:    链表头插
 * next:    下一个符号*/
#define SYMBOL_COMMON   \
    int kind;           \
    char *name;         \
    char *aname;        \
    Type ty;            \
    int level;          \
    int sclass;         \
    int ref;            \
    int defined     :1; \
    int addressed   :1; \
    int needwb      :1; \
    int unvalid     :1; \
    int unused      :28;\
    union value val;    \
    struct symbol *reg; \
    struct symbol *link;\
    struct symbol *next

/* 一个基本块 */
typedef struct bblock *BBlock;
/* 初始化一块内存 */
typedef struct initData *InitData;

/* 描述一个符号 */
typedef struct symbol {

    SYMBOL_COMMON;
} *Symbol;

/* 一个值的定义
 * dst:         是地址时,表示指向
 * op:          操作
 * scr1,scr2:   操作数
 * ownBB:       内存块
 * link: */
typedef struct valueDef {
    
    Symbol  dst;
    int     op;
    Symbol  src1, src2;
    BBlock  ownBB;
    struct valueDef *link;
} *ValueDef;

/* 被使用的值 */
typedef struct valueUse {

    ValueDef def;
    struct valueUse *next;
} *ValueUse;

/* 描述一个变量 */
typedef struct variableSymbol {
    
    SYMBOL_COMMON;
    /* 变量所有的一块内存 */
    InitData idata;
    /* 变量定义 */
    ValueDef def;
    /* 变量使用的值 */
    ValueUse uses;
    int offset;
} *VariableSymbol;

/* 描述一个函数的符号 */
typedef struct functionSymbol {
    
    SYMBOL_COMMON;
    /* 函数的参数 */
    Symbol params;
    /* 局部变量 */
    Symbol locals;
    /* 从函数参数开始 */
    Symbol *lastv;
    int nbblock;
    BBlock entryBB;
    BBlock exitBB;
    /* 存储变量的值的hash 列表 */
    ValueDef valNumTable[16];
} *FunctionSymbol;

/* 描述一个符号表 */
typedef struct table {

    /* 符号链表 */
    Symbol *buckets;
    /* 符号的级别(范围) */
    int level;
    /* 指向外部的符号表 */
    struct table *outer;
} *Table;

#define AsVar(sym)  ((VariableSymbol)sym)
#define AsFunc(sym) ((FunctionSymbol)sym)

/* 保存函数的符号表 */
extern Symbol Functions;
/* 表示嵌套层次 */
extern int Level;
/* 一个函数的符号表(函数头部信息,函数内部信息等) */
extern FunctionSymbol FSYM; 
extern unsigned int TempNum;

/* 初始化符号表 */
void InitSymbolTable (void);
/* 查找某个类型 */
Symbol LookupTag (const char *name);
/* 添加新类型到符号表 */
Symbol AddTag (char *name, Type ty);
/* 添加枚举常量到符号表 */
Symbol AddEnumConstant (char *name, Type ty, int val);
/* 查找某个标示符 */
Symbol LookupID (const char *name);
/* 添加一个函数到符号表中 */
Symbol AddFunction (char *name, Type ty, int sclass);
/* 进入一个模块 */
void EnterScope (void);
/* 退出一个模块 */
void ExitScope (void);
/* 添加变量到符号表中 */
Symbol AddVariable (char *name, Type ty, int sclass);
/* 添加一个typedef 别名到符号表中 */
Symbol AddTypedefName (char *name, Type ty);
/* 添加一个字符串到符号表中 */
Symbol AddString (Type ty, String str);
/* 创建标签 */
Symbol CreateLabel (void);
/* 查找系统调用号 */
int LookupSysCall (const char *name);
/* 构造int 类型常量 */
Symbol IntConstant (int i);
/* 创建函数内部的临时变量 */
Symbol CreateTemp (Type ty);
/* 创建偏移符号 */
Symbol CreateOffset (Type ty, Symbol base, int coff);
/* 添加常量到符号表中 */
Symbol AddConstant (Type ty, union value val);

Symbol AddDefine (char *name, char *content);
Symbol LookupDefine (const char *name);
Symbol DeleteDefine (char *name);

#endif
