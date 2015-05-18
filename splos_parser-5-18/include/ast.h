/*************************************************************************
	> File Name: ast.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月19日 星期三 21时06分08秒

	> Description: 
 ************************************************************************/

#ifndef __AST_H
#define __AST_H

/* 节点的类型 */
enum nodeKind 
{ 
    /* 声明*/
	NK_TranslationUnit,     NK_Function,           NK_Declaration,
	NK_TypeName,            NK_Specifiers,         NK_Token,				
	NK_TypedefName,         NK_EnumSpecifier,      NK_Enumerator,			
	NK_StructSpecifier,     NK_UnionSpecifier,     NK_StructDeclaration,	
	NK_StructDeclarator,    NK_PointerDeclarator,  NK_ArrayDeclarator,		
	NK_FunctionDeclarator,  NK_ParameterTypeList,  NK_ParameterDeclaration,
	NK_NameDeclarator,      NK_InitDeclarator,     NK_Initializer,
	
    /* 表达式 */
	NK_Expression,

    /* 语句 */
	NK_ExpressionStatement, NK_LabelStatement,     NK_CaseStatement,		
	NK_DefaultStatement,    NK_IfStatement,        NK_SwitchStatement,		
	NK_WhileStatement,      NK_DoStatement,        NK_ForStatement,		
	NK_GotoStatement,       NK_BreakStatement,     NK_ContinueStatement,		
	NK_ReturnStatement,     NK_CompoundStatement,  

    /* 并行语句 */
    NK_ParallelStatement
};

/* 表达式节点 */
typedef struct astExpression        *AstExpression;
/* 声明 */
typedef struct astDeclaration       *AstDeclaration;
typedef struct astTypeName          *AstTypeName;
typedef struct astTranslationUnit   *AstTranslationUnit;
typedef struct astStatement         *AstStatement;

/* 节点的公共属性 
 * kind: 节点的类型
 * next: 下一个节点
 * coord:节点的坐标*/
#define AST_NODE_COMMON     \
    int kind;               \
    struct astNode *next;   \
    struct coord coord

/* 描述一个普通的节点 */
typedef struct astNode {
    
    AST_NODE_COMMON;
} *AstNode;

/* 变量初始化 */
typedef struct initData {

    /* 相对变量内存起始的偏移 */
    int offset;
    /* 用以初始化内存的表达式,表达式的类型指定了内存大小 */
    AstExpression expr;
    /* 下一个数据块 */
    struct initData *next;
} *InitData;

/* 标号:函数中的标号,作用域是整个函数 */
typedef struct label {

    /* 在文件中的坐标,这样当函数结束时,可以检查未定义的标号,报告错误 */
    struct coord coord;
    /* 标号名 */
    char *id;
    /* 标号的引用次数 */
    int ref;
    /* 标号是否被定义 */
    int defined;
    /* 标号对应的基本块,中间代码生成使用 */
    BBlock respBB;
    /* 下一个标 */
    struct label *next;
} *Label;

/* 创建一个节点 */
#define CREATE_AST_NODE(p, k)   \
    CALLOC (p);                 \
    p->kind = NK_ ##k;          \
    p->coord = TokenCoord;

/* 得到下一个词法单元 */
#define NEXT_TOKEN  CurrentToken = GetNextToken ()

/* 检查当前的token是否是当前所期望的 */
void Expect (int tok);
/* 在toks 中查找当前tok */
int CurrentTokenIn (int toks[]);

void SkipTo(int toks[], const char *einfo);


/* 以下实现在decl.c 中 */
extern int FIRST_Declaration[];
int IsTypeName(int tok);
AstTypeName ParseTypeName (void);
AstDeclaration ParseDeclaration(void);
AstTranslationUnit ParseTranslationUnit(char *filename);
/* 一个木块结束,去除模块内的所有重载 */
void PostCheckTypedef(void);
/* declchk.c 对decl中生成的节点进行语义检查  */
void CheckTranslationUnit(AstTranslationUnit transUnit);


/* 以下实现在expr.c 中 */
extern char *OPNames[];
/* 赋值表达式 */
AstExpression ParseAssignmentExpression(void);
AstExpression ParseConstantExpression(void);
AstExpression ParseExpression (void);
AstExpression CheckConstantExpression(AstExpression expr);


/* 以下实现在stmt.c 中 */
/* 处理复合语句 */
AstStatement ParseCompoundStatement(void);

/* 以下实现在dumpast.h 中 */
/* 打印语法树 */
void DumpTranslationUnit (AstTranslationUnit transUnit);
void Translate(AstTranslationUnit transUnit);

/* 打印中间代码 */
void DAssemTranslationUnit (AstTranslationUnit transUnit);

extern int CurrentToken;

#endif
