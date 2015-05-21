/*************************************************************************
	> File Name: decl.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月20日 星期四 13时10分19秒

	> Description: 
 ************************************************************************/

#ifndef __DECL_H
#define __DECL_H

/* 抽象的和具体的 */
enum {DEC_ABSTRACT = 0x01, DEC_CONCRETE = 0x02};
/* 指针, 数组和函数 */
enum {POINTER_TO, ARRAY_OF, FUNCTION_RETURN};

/* 声明的公共属性 */
/*
 * dec :若是数组,则表示中括号前的ID表达式
 * */
#define AST_DECLARATOR_COMMON   \
    AST_NODE_COMMON;            \
    struct astDeclarator *dec;  \
    char *id;                   \
    TypeDerivList tyDrvList;

/* 描述类型说明符 */
typedef struct typeDerivList {

    /* 表示数组, 函数返回值, 指针 */
    int ctor;
    union {
        /* 若是数组则表示数组的长度 */
        int len;
        /* 若是指针则表示指针限定符 */
        int qual;
        /* 函数头部信息 */
        Signature sig;
    };
    struct typeDerivList *next;
} *TypeDerivList;

/* 声明中的一个节点 */
typedef struct astDeclarator {

    AST_DECLARATOR_COMMON;
} *AstDeclarator;

/* typedef 定义的类型 */
typedef struct tdname {

    /* 类型名 */
    char *id;
    /* 表示嵌套的层次 */
    int level;
    /* 重载 */
    int overload;
} *TDName;

/* 描述一个完整的定义eg: static const struct Type A; */
typedef struct astSpecifiers {

    AST_NODE_COMMON;
    /* 存储类型符 */
    AstNode stgClasses;
    /* 类型限定符(const, volatile) */
    AstNode tyQuals;
    /* 类型 */
    AstNode tySpecs;
    /* 存储类型的类别码 */
    int sclass;
    Type ty;
} *AstSpecifiers;

/* 声明和初始化块 */
typedef struct astDeclaration {
    
    AST_NODE_COMMON;
    /* 一个变量的完整定义 */
    AstSpecifiers specs;
    /* 初始化块 */
    AstNode initDecs;
} *AstDeclaration;

typedef struct astToken {
    
    AST_NODE_COMMON;
    int token;
} *AstToken;

/* 变量初始化的数据结构 */
typedef struct astInitializer {
    
    AST_NODE_COMMON;
    /* 初始化块是否含有括号 */
    int lbrace;
    union {
        /* 有花括号时,存储括号里的每一项 */
        AstNode initials;
        /* 没有花括号是存储表达式 */
        AstExpression expr;
    };
    /* 保存调整好的数据 */
    InitData idata;
} *AstInitializer;

/* 定义初始化(包括第二部分和初始化模块) */
typedef struct astInitDeclarator {
    
    AST_NODE_COMMON;
    /* 定义的第二部分 */
    AstDeclarator dec;
    /* 初始化 */
    AstInitializer init;
} *AstInitDeclarator;

/* typedef 类型重命名 */
typedef struct astTypedefName {
    
    AST_NODE_COMMON;
    char *id;
    Symbol sym;
} *AstTypedefName;

/* 构造类型 */
typedef struct astStructSpecifier {
    
    AST_NODE_COMMON;
    /* struct 紧跟的id */
    char *id;
    /* 描述struct id 后的字段列表 */
    AstNode stDecls;
} *AstStructSpecifier;

/* 构造类型的一个字段 */
typedef struct astStructDeclaration {
    
    AST_NODE_COMMON;
    /* 字段的第一部分 */
    AstSpecifiers specs;
    /* 字段的第二三部分 */
    AstNode stDecs;
} *AstStructDeclaration;

/* struct 结构体中位段的描述eg:
 * struct A {
 *  int a : 16;
 *  int b : 16;
 * };
  */
/* 构造类型的第二三部分 */
typedef struct astStructDeclarator {
    
    AST_NODE_COMMON;
    /* 第二部分 */
    AstDeclarator dec;
    /* 表达式(16) */
    AstExpression expr;
} *AstStructDeclarator;

/* 描述指针(*后边的内容) */
typedef struct astPointerDeclarator {

    AST_DECLARATOR_COMMON;
    /* 限定符 */
    AstNode tyQuals;
} *AstPointerDeclarator;

/* 描述数组(中括号里的内容) */
typedef struct astArrayDeclarator {

    AST_DECLARATOR_COMMON;
    AstExpression expr;
} *AstArrayDeclarator;

/* 参数类型列表的描述 */
typedef struct astParameterTypeList {

    AST_NODE_COMMON;
    /* 参数类型列表 */
    AstNode paramDecls;
    /* 不定参 */
    int ellipse;
} *AstParameterTypeList;

/* 单个参数的描述 */
typedef struct astParameterDeclaration {
    
    AST_NODE_COMMON;
    /* 一个完整顶一个的描述 */
    AstSpecifiers specs;
    AstDeclarator dec;
} *AstParameterDeclaration;

/* 函数括号里的参数描述 */
typedef struct astFunctionDeclarator {

    AST_DECLARATOR_COMMON;
    /* 实参列表 */
    Vector ids;
    /* 参数类型列表 */
    AstParameterTypeList paramTyList;
    /* 该函数是否定义 */
    int partOfDef;
    /* 参数列表 */
    Signature sig;
} *AstFunctionDeclarator;

/* 枚举类型结构描述 */
typedef struct astEnumSpecifier {

    AST_NODE_COMMON;
    /* 枚举类型的名字 */
    char *id;
    /* 枚举元素的列表 */
    AstNode enumers;
} *AstEnumSpecifier;

/* 枚举里边的一个元素 */
typedef struct astEnumerator {

    AST_NODE_COMMON;
    /* 一个元素的名字 */
    char *id;
    /* 该元素的表达式 */
    AstExpression expr;
} *AstEnumerator;

/* 类型节点的数据结构 */
typedef struct astTypeName {

    AST_NODE_COMMON;
    /* 一个变量的完整定义描述 */
    AstSpecifiers specs;
    AstDeclarator dec;
} *AstTypeName;

/* 语句翻译的数据结构 */
typedef struct astTranslationUnit {
    
    AST_NODE_COMMON;
    /* 语句链表 */
    AstNode extDecls;
} *AstTranslationUnit;

/* 描述一个函数 */
typedef struct astFunction {

    AST_NODE_COMMON;
    /* 函数描述的第一部分 */
    AstSpecifiers specs;
    /* 函数定义的第二部分 */
    AstDeclarator dec;
    /* 函数参数类型等信息 */
    AstFunctionDeclarator fdec;
    /* 参数的第三部分 */
    AstNode decls;
    /* 函数体 */
    AstStatement stmt;
    FunctionSymbol fsym;
    Label labels;
    /* 循环 */
    Vector loops;
    /* switch */
    Vector swtches;
    /* break 的跳出点 */
    Vector breakable;
    /* 是否有返回值 */
    int hasReturn;
} *AstFunction;



Type CheckTypeName (AstTypeName tname);
void CheckLocalDeclaration (AstDeclaration decl, Vector v);

extern void StructConstant0 (void);

/* 当前所在的函数 */
extern AstFunction CURRENTF;

#endif
