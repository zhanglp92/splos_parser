/*************************************************************************
	> File Name: decl.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月20日 星期四 12时20分50秒

	> Description: C语言声明的语法分析
 ************************************************************************/


#include "pcl.h"
#include "ast.h"
#include "decl.h"
#include "grammer.h"
#include "stmt.h"


/* 声明的第一部分(修饰符, 类型) */
static int FIRST_StructDeclaration[]    = {FIRST_DECLARATION, 0};
static int FF_StructDeclaration[]       = { FIRST_DECLARATION, TK_RBRACE, 0 };
static int FIRST_ExternalDeclaration[]  = { FIRST_DECLARATION, TK_MUL, TK_LPAREN, TK_IMPORT, 0 };


int FIRST_Declaration[] = {FIRST_DECLARATION, 0};
/* 分别表示所有定义的类型名,和前作用域中被重载了的类型名*/
static Vector TypedefNames, OverloadNames;

/* 下边三个函数解析一个完整的变量定义eg: 
 * auto const struct Type *const *p(const struct A a, ...); */
/* 解析从开始到类型名Type之前的部分(包括Type,调用下一个函数) */
/* 第一部分 */
static AstSpecifiers ParseDeclarationSpecifiers (void);

/* 解析从类型名Type之后到变量名之前的部分,调用ParsePostfixDeclarator 函数处理后边的部分 */
/* 第二部分 */
static AstDeclarator ParseDeclarator (int kind);

/* 变量名之后的部分(后缀), 函数头部(不包括函数体) */
/* 第三部分 */
static AstDeclarator ParsePostfixDeclarator (int kind);

/* 得到描述符的ID */
static char* GetOutermostID (AstDeclarator dec) 
{
    if (NULL == dec) return NULL;

    if (NK_NameDeclarator == dec->kind) 
        return dec->id;

    return GetOutermostID (dec->dec);
}

/* 判断是否为typedef 类型 */
static int IsTypedefName (const char *id) 
{
    Vector v = TypedefNames;
    TDName tn;

    /* 遍历所有typedef 定义的类型名 */
    FOR_EACH_ITEM (TDName, tn, v) 
        /* 判断此类型是否被定义过 */
        if (tn->id == id && tn->level <= Level && !tn->overload) 
            return 1;
    ENDFOR

    /* 此类型没有被定义过 */
    return 0;
}

/* 检查Typedef 定义的类型名 */
static void CheckTypedefName (int sclass, char *id) 
{
    Vector v;
    TDName tn;

    if ( !id ) return ;

    /* 所有typedef 类型名 */
    v = TypedefNames;

    /* 如果是typedef 定义的类型 */
    if (TK_TYPEDEF == sclass) {
    
        /* 遍历所有类型名 */
        FOR_EACH_ITEM (TDName, tn, v) 
            /* 若找到,则将嵌套值设置为最小 */
            if (tn->id == id) {
                /* Level 当前嵌套值 */
                if (Level < tn->level)
                    tn->level = Level;
                return ;
            }
        ENDFOR

        /* 没有,则将此类型添加进去 */
        ALLOC (tn);
        tn->id      = id;
        tn->level   = Level;
        tn->overload = 0;
        INSERT_ITEM (v, tn);
    } else {
        
        FOR_EACH_ITEM (TDName, tn, v) 
            /* 若名字相同,并且是内嵌的则标记为重载 */
            if (tn->id == id && Level > tn->level) {
    
                /* 将外层的标记为重载,并加入到OverloadNames 集合中 */
                tn->overload = 1;
                INSERT_ITEM (OverloadNames, tn);
            }
        ENDFOR
    }
}

/* 检查Typedef 类型 */
static void PreCheckTypedef (AstDeclaration decl)
{
    AstNode p;
    int sclass = 0;

    /* 取得类别码 */
    if (decl->specs->stgClasses) 
        sclass = ((AstToken)decl->specs->stgClasses)->token;

    /* 初始化块 */
    for (p = decl->initDecs; p; p = p->next) 
        CheckTypedefName (sclass, GetOutermostID (((AstDeclarator)p)->dec));
}

/* 复合语句结束时,重置OverloadNames 中的所有类型名的重载状态 */
void PostCheckTypedef (void) 
{
    TDName tn;

    FOR_EACH_ITEM (TDName, tn, OverloadNames) 
        tn->overload = 0;
    ENDFOR
    OverloadNames->len = 0;
}

/* 变量初始化解析函数(采用递归的方式) */
static AstInitializer ParseInitializer (void)
{
    AstInitializer init;
    AstNode *tail;

    /* 创建一个节点 */
    CREATE_AST_NODE (init, Initializer);
    /* 如果当前是左大括号 */
    if (TK_LBRACE == CurrentToken) {
    
        init->lbrace = 1;
        NEXT_TOKEN;
        /* 解析括号里边的第一个模块 */
        init->initials = (AstNode)ParseInitializer ();
        tail = &init->initials->next;
        /* 如果当前是逗号 */
        while (TK_COMMA == CurrentToken) {
        
            NEXT_TOKEN;
            /* 如果是右大括号则,一个小的初始化模块结束 */
            if (TK_RBRACE == CurrentToken) 
                break;
            *tail = (AstNode)ParseInitializer ();
            tail = &(*tail)->next;
        }
        /* 期望遇到右大括号 */
        Expect (TK_RBRACE);
    } else {
    
        /* 直到里边的最小模块(不含花括号) */
        init->lbrace = 0;
        init->expr = ParseAssignmentExpression ();
    }
    return init;
}

/* 处理变量名,函数名,typedef名等 */
/*
 * direct-declarator:
 *      ID, (declarator)
 *
 *  direct-abstract-declarator:
 *      (abstract-declarator)
 *      nil(无)
 * */
static AstDeclarator ParseDirectDeclarator (int kind) 
{
    AstDeclarator dec;

    /* 左小括号,描述小括号中的内容 */
    if (TK_LPAREN == CurrentToken) {

        NEXT_TOKEN;
        dec = ParseDeclarator (kind);
        Expect (TK_RPAREN);
        return dec;
    }

    CREATE_AST_NODE (dec, NameDeclarator);
    if (TK_ID == CurrentToken) {
    
        if (DEC_ABSTRACT == kind) {

            Error(&TokenCoord, "Identifier is not permitted in the abstract declarator");
        }

        dec->id = TokenValue.p;
        NEXT_TOKEN;
    } else if (DEC_CONCRETE == kind) {
    
        Error(&TokenCoord, "Expect identifier");
    }

    return dec;
}

/* 单个参数的描述 */
/* 
 * parameter-declaration:
 *      declaration-specifiers declarator           (第一部分)
 *      declaration-specifiers abstract-declarator  (第二部分)
 * */
static AstParameterDeclaration ParseParameterDeclaration (void)
{
    AstParameterDeclaration paramDecl;

    CREATE_AST_NODE (paramDecl, ParameterDeclaration);
    /* 第一部分 */
    paramDecl->specs = ParseDeclarationSpecifiers ();
    /* 第二部分 */
    paramDecl->dec = ParseDeclarator (DEC_ABSTRACT | DEC_CONCRETE);

    return paramDecl;
}


/* 参数列表类型的描述 */
/**
 *  parameter-type-list:        (参数列表)
 *		parameter-list          (参数)
 *		parameter-list , ...    (参数, 不定参)
 *
 *  parameter-list:
 *		parameter-declaration
 *		parameter-list , parameter-declaration
 *
 *  parameter-declaration:
 *		declaration-specifiers declarator           (第一部分 第二部分)
 *		declaration-specifiers abstract-declarator  (第一部分 第二部分)
 */
AstParameterTypeList ParseParameterTypeList (void) 
{
    AstParameterTypeList paramTyList;
    AstNode *tail;

    CREATE_AST_NODE (paramTyList, ParameterTypeList);

    /* 解析一个完整的参数 */
    paramTyList->paramDecls = (AstNode)ParseParameterDeclaration ();
    tail = &paramTyList->paramDecls->next;
    /* 如果遇到逗号则继续扫描参数 */
    while (TK_COMMA == CurrentToken) {

        NEXT_TOKEN;
        /* 当前是不定参 */
        if (TK_ELLIPSE == CurrentToken) {
            
            paramTyList->ellipse = 1;
            NEXT_TOKEN;
            break;
        }
        *tail = (AstNode)ParseParameterDeclaration ();
        tail = &(*tail)->next;
    }
    return paramTyList;
}


/* 解析变量名之后的部分 */
/**
 *  postfix-declarator:
 *		direct-declarator
 *		postfix-declarator [ [constant-expression] ]    (数组)
 *		postfix-declarator ( parameter-type-list)       (形参列表)
 *		postfix-declarator ( [identifier-list] )        
 *
 *  postfix-abstract-declarator:
 *		direct-abstract-declarator
 *		postfix-abstract-declarator ( [parameter-type-list] )   (实参列表)
 *		postfix-abstract-declarator [ [constant-expression] ]
 */
static AstDeclarator ParsePostfixDeclarator (int kind)
{
    AstDeclarator dec = ParseDirectDeclarator (kind);

    while (1) {
        
        /* 左中括号表示数组 */
        if (TK_LBRACKET == CurrentToken) {

            AstArrayDeclarator arrDec;

            CREATE_AST_NODE (arrDec, ArrayDeclarator)
            arrDec->dec = dec;

            NEXT_TOKEN;
            if (TK_RBRACKET != CurrentToken) {

                /* 中括号的数组长度表达式 */
                arrDec->expr = ParseConstantExpression ();
            }
            Expect (TK_RBRACKET);

            /* dec 表示整个数组的表示(第二三部分) */
            dec = (AstDeclarator)arrDec;
        } else if (TK_LPAREN == CurrentToken) {

            /* 函数的形参列表 */
            AstFunctionDeclarator funDec;

            CREATE_AST_NODE (funDec, FunctionDeclarator);
            funDec->dec = dec;

            NEXT_TOKEN;
            /* 小括号中首先遇到的参数类型 */
            if (IsTypeName (CurrentToken)) {
    
                /* 处理形参 */
                funDec->paramTyList = ParseParameterTypeList ();
            } else {
 
                /* 处理实参 */
                funDec->ids = CreateVector (4);
                if (TK_ID == CurrentToken) {
                    
                    /* 将实参添加到对象的属性中 */
                    INSERT_ITEM (funDec->ids, TokenValue.p);
                    NEXT_TOKEN;
                    while (TK_COMMA == CurrentToken) {

                        NEXT_TOKEN;
                        if (TK_ID == CurrentToken) 
                            INSERT_ITEM (funDec->ids, TokenValue.p);
                        Expect (TK_ID);
                    }
                }
            }
            Expect (TK_RPAREN);
            dec = (AstDeclarator)funDec;
        } else return dec;
    }
}

/* 第二部分 */
/**
 *  abstract-declarator:    (抽象定义)
 *		pointer             (指针)
 *		[pointer] direct-abstract-declarator
 *
 *  direct-abstract-declarator:
 *		( abstract-declarator )                                 (抽象类型)
 *		[direct-abstract-declarator] [ [constant-expression] ]  (指针数组)      
 *		[direct-abstract-declarator] ( [parameter-type-list] )  (函数指针)
 *
 *  declarator:
 *		pointer declarator
 *		direct-declarator
 *
 *  direct-declarator:
 *		ID                                              (用户标示符)
 *		( declarator )
 *		direct-declarator [ [constant-expression] ]     (数组)
 *		direct-declarator ( parameter-type-list )       (函数声明,函数头部)
 *		direct-declarator ( [identifier-list] )         
 *
 *  pointer:
 *		* [type-qualifier-list]                         (* 类型限定符列表)
 *		* [type-qualifier-list] pointer
 *
 * 重新修改了一下上边的产生式
 *  abstract-declarator:
 *		* [type-qualifer-list] abstract-declarator
 *		postfix-abstract-declarator
 *	
 *  postfix-abstract-declarator:
 *		direct-abstract-declarator
 *		postfix-abstract-declarator [ [constant-expression] ]   (指针数组)
 *		postfix-abstrace-declarator( [parameter-type-list] )    (函数指针)
 *		
 *  direct-abstract-declarator:
 *		( abstract-declarator )
 *		NULL
 *
 *  declarator:     (定义)
 *		* [type-qualifier-list] declarator
 *		postfix-declarator
 *
 *  postfix-declarator:
 *		direct-declarator
 *		postfix-declarator [ [constant-expression] ]            (数组)
 *		postfix-declarator ( parameter-type-list)               (函数声明)
 *		postfix-declarator ( [identifier-list] )
 *
 *  direct-declarator:
 *		ID
 *		( declarator )
 *
 *	The declartor is similar as the abstract declarator, we use one function
 *	ParseDeclarator() to parse both of them. kind indicate to parse which kind
 *	of declarator. The possible value can be:
 *	DEC_CONCRETE: parse a declarator
 *	DEC_ABSTRACT: parse an abstract declarator
 *	DEC_CONCRETE | DEC_ABSTRACT: both of them are ok
 */
static AstDeclarator ParseDeclarator (int kind)
{
    /* 识别类型后边,变量前边的 * 和限定符 */
    if (TK_MUL == CurrentToken) {

        AstPointerDeclarator ptrDec;
        AstToken    tok;
        AstNode     *tail;

        /* 创建指针节点 */
        CREATE_AST_NODE (ptrDec, PointerDeclarator);
        tail = &ptrDec->tyQuals;

        NEXT_TOKEN;
        /* 判断星号后边是否还有限定词 */
        while (TK_CONST == CurrentToken || TK_VOLATILE == CurrentToken) {
        
            CREATE_AST_NODE (tok, Token);
            tok->token  = CurrentToken;
            *tail       = (AstNode)tok;
            tail        = &tok->next;
            NEXT_TOKEN; 
        }
        /* 继续扫描是否有多维指针 */
        ptrDec->dec = ParseDeclarator (kind);

        return (AstDeclarator)ptrDec;
    }
    /* 调用第三部分 */
    return ParsePostfixDeclarator (kind);
}

/* 构造类型中的一个字段的,第二三部分 */
/* 
 * struct-declarator:
 *      declarator
 *      [declarator]: constant-expression
 * */
static AstStructDeclarator ParseStructDeclarator (void)
{
    AstStructDeclarator stDec;
    CREATE_AST_NODE (stDec, StructDeclarator);

    /* 不是冒号 */
    if (TK_COLON != CurrentToken) {
    
        /* 声明的第二部分(第二部分中调用第三部分) */
        stDec->dec = ParseDeclarator (DEC_CONCRETE);
    }

    /* 是位段类型的一个字段 */
    if (TK_COLON == CurrentToken) {

        NEXT_TOKEN;
        stDec->expr = ParseConstantExpression ();
    }

    return stDec;
}

/* 构造类型中一个完整字段的解析 */
/* 
 * struct-declaration:
 *      specifier-qualifer-list struct-declarator-list ;    (一个完整的字段)
 *
 * specifier-qualifier-list:
 *      type-specifier [specifier-qualifier-list]           (类型名)
 *      type-qualifier [specifier-qualifier-list]           (类型限定符)
 *
 * struct-declarator-list:
 *      struct-declarator
 *      struct-declarator-list , struct-declarator
 */
static AstStructDeclaration ParseStructDeclaration (void) 
{
    /* 保存struct 声明 */
    AstStructDeclaration stDecl;
    AstNode *tail;
    
    /* 创建一个字段的节点 */
    CREATE_AST_NODE (stDecl, StructDeclaration);
    /* 解析第一个字段的第一部分 */
    stDecl->specs = ParseDeclarationSpecifiers ();

    /* 结构中定义字段不应该有存储类说明符 */
    if (stDecl->specs->stgClasses) {

        Error (&stDecl->coord, "Struct/union member should not have storage class");
        stDecl->specs->stgClasses = NULL;
    }

    /* 没有限定符,和类型 */
    if (!stDecl->specs->tyQuals && !stDecl->specs->tySpecs) {
    
        Error(&stDecl->coord, "Expect type specifier or qualifier");
    }

    /* 如果是分号,扫描结束(该字段为空) */
    if (TK_SEMICOLON == CurrentToken) {

        NEXT_TOKEN;
        return stDecl;
    }

    /* 解析该字段的第二部分(及第三部分) */
    stDecl->stDecs = (AstNode)ParseStructDeclarator ();
    tail = &stDecl->stDecs->next;
    /* 如果是逗号继续 */
    while (TK_COMMA == CurrentToken) {

        NEXT_TOKEN;
        *tail = (AstNode)ParseStructDeclarator ();
        tail = &(*tail)->next;
    }
    /* 该字段结束期望分号 */
    Expect (TK_SEMICOLON);

    return stDecl;
}

/* 构造类型解析函数 */
/*
* struct-or-union-specifier:    
*       struct-or-union [identifier] { struct-declaration-list  }   (结构体的定义)
*       struct-or-union identifier                                  (变量的定义)
*
* struct-or-union:
*       struct, union
*
* struct-declaration-list:  (字段列表)
*       struct-declaration 
*       struct-declaration-list struct-declaration
* */
static AstStructSpecifier ParseStructOrUnionSpecifier (void) 
{
    /* 描述一个struct 或 union */
    AstStructSpecifier  stSpec;
    AstNode *tail;

    /* 创建struct 或者 union 节点 */
    CREATE_AST_NODE (stSpec, StructSpecifier);
    if (TK_UNION == CurrentToken) 
        stSpec->kind = NK_UnionSpecifier;

    NEXT_TOKEN;
    switch (CurrentToken) {
        
        case TK_ID:
            /* 保存struct 或union 的名字 */
            stSpec->id = TokenValue.p;
            NEXT_TOKEN;
            /* 如果是左大括号,则跳转处理 */
            if (TK_LBRACE == CurrentToken) 
                goto lbrace;
            /* 如果不是大括号,则扫描结束 */
            return stSpec;

        /* 处理左花括号里边的内容 */
        case TK_LBRACE:
lbrace:
            NEXT_TOKEN;
            /* 如果下一个token是右花括号,则结束 */
            if(TK_RBRACE == CurrentToken) {
                
                NEXT_TOKEN;
                return stSpec;
            }

            tail = &stSpec->stDecls;
            while (CurrentTokenIn (FIRST_StructDeclaration)) {
            
                /* 构造类型中一个完整的字段 */
                *tail = (AstNode)ParseStructDeclaration ();
                tail = &(*tail)->next;
                SkipTo (FF_StructDeclaration, "the start of struct declaration or }");
            }
            /* 构造类型结束 */
            Expect (TK_RBRACE);
            return stSpec;

        default:
            Error(&TokenCoord, "Expect identifier or { after struct/union");
            return stSpec;
    }
}


/* 扫描枚举列表里边的元素eg: enum {A = 1, B, C}
 * 描述里边的A = 1 */
/**
 * enumerator:  (枚举)
 *      enumeration-constant                        (枚举元素,没有初始化)
 *      enumeration-constant = constant-expression  (枚举元素,有初始化)
 *
 * enumeration-constant:    (枚举元素)
 *      constantidentifier  (常量标示符)
*/
static AstEnumerator ParseEnumerator (void) 
{
    /* 保存一个枚举类型 */
    AstEnumerator enumer;

    /* 创建一个节点 */
    CREATE_AST_NODE (enumer, Enumerator);
    /* 若开始不是标示符,则错误 */
    if (TK_ID != CurrentToken) {

        Error (&TokenCoord, "The eumeration constant must be identifie");
        return enumer;
    }

    /* 存储标示符和他的值 */
    enumer->id = TokenValue.p;
    NEXT_TOKEN;
    if (TK_ASSIGN == CurrentToken) {

        NEXT_TOKEN;
        /* 取得某个元素的值 */
        enumer->expr = ParseConstantExpression ();
    }
    return enumer;
}

/* 枚举类型的解析函数 */
/**
 *  enum-specifier
 *		enum [identifier] { enumerator-list }
 *		enum [identifier] { enumerator-list , }
 *		enum identifier
 *
 *  enumerator-list:
 *		enumerator
 *		enumerator-list , enumerator
 */
static AstEnumSpecifier ParseEnumSpecifier (void) 
{
    /* 保存一个枚举类型 */
    AstEnumSpecifier enumSpec;
    AstNode *tail;

    /* 创建一个保存枚举类型的节点 */
    CREATE_AST_NODE (enumSpec, EnumSpecifier);
    
    NEXT_TOKEN;
    /* 处理枚举类型的名字 */
    if (TK_ID == CurrentToken) {
        
        enumSpec->id = TokenValue.p;
        NEXT_TOKEN;
    } 

    if (TK_LBRACE == CurrentToken) {

        NEXT_TOKEN;
        /* 枚举完成直接返回 */
        if (TK_RBRACE == CurrentToken) 
            return enumSpec;

        /* 扫描枚举里边的元素 */
        enumSpec->enumers = (AstNode)ParseEnumerator ();
        tail = &enumSpec->enumers->next;
        while (TK_COMMA == CurrentToken) {

            NEXT_TOKEN;
            if (TK_RBRACE == CurrentToken) 
                break;
            *tail = (AstNode)ParseEnumerator ();
            tail = &(*tail)->next;
        } 
        Expect (TK_RBRACE);
    } else {
        
        Error (&TokenCoord, "Expect identifier or { after enum");
    }
    return enumSpec;
}

/* 第一部分(存储类型说明符号->类型限定符->类型) eg:
 * static const struct Type ; */
/**
 *  declaration-specifiers:
 *		storage-class-specifier [declaration-specifiers]    (存储类型说明符号)
 *		type-specifier [declaration-specifiers]             (类型)
 *		type-qualifier [declaration-specifiers]             (类型限定符)
 *
 *  storage-class-specifier(存储类说明符):
 *		auto, register, static, extern, typedef
 *
 *  type-qualifier(类型限定符):
 *		const, volatile
 *
 *  type-specifier(类型名):
 *		void, char, short, int, long, float
 *		double, signed, unsigned
 *		struct-or-union-specifier
 *		enum-specifier, typedef-name
 */
static AstSpecifiers ParseDeclarationSpecifiers (void) 
{
    /* 保存整个声明 */
    AstSpecifiers   specs;
    /* 临时保存节点 */
    AstToken tok;
    /* 保存存储类型说明符,类型限定符,类型的链表 */
    AstNode *scTail, *tqTail, *tsTail;
    /* 是否已经有了类型, 用来区分typedef 重命名的类型名和变量名 */
    int seeTy = 0;

    /* 创建一个保存整个声明节点 */
    CREATE_AST_NODE (specs, Specifiers);
    /* 以上三组数据都已链表的形式存储 */
    scTail = &specs->stgClasses;
    tqTail = &specs->tyQuals;
    tsTail = &specs->tySpecs;

    while (1) {

        switch (CurrentToken) {

            /* 存储类说明符 */
            case TK_AUTO:   case TK_REGISTER: TK_EXTERN: 
            case TK_STATIC: case TK_TYPEDEF: {

                /* 创建存储类型修饰符的节点 */
                CREATE_AST_NODE (tok, Token);
                /* 直接保存关键字的类别码 */
                tok->token  = CurrentToken;
                /* 将其接入到链表 */
                *scTail     = (AstNode)tok;
                scTail      = &tok->next;
                NEXT_TOKEN;
           }break;

            /* 类型限定符 */
            case TK_CONST: case TK_VOLATILE: {
    
                /* 和上边一样 */
                CREATE_AST_NODE (tok, Token);
                tok->token = CurrentToken;
                *tqTail = (AstNode)tok;
                tqTail = &tok->next;
                NEXT_TOKEN;
            }break; 

            /* 类型名(基本类型) */
            case TK_VOID:   case TK_CHAR:   case TK_SHORT: 
            case TK_INT:    case TK_INT64:  case TK_LONG: 
            case TK_FLOAT:  case TK_DOUBLE: case TK_UNSIGNED: 
            case TK_SIGNED: {
    
                CREATE_AST_NODE (tok, Token);
                tok->token = CurrentToken;
                *tsTail = (AstNode)tok;
                tsTail = &tok->next;
                /* 标志是否已经扫描到了类型名 */
                seeTy = 1;
                NEXT_TOKEN;
            }break;
        
            /* 可能是类型名,或者其他 */
            case TK_ID: {

                /* 类型别名 */
                if (!seeTy && IsTypedefName (TokenValue.p)) {

                    /* typedef 重命名过的名字 */
                    AstTypedefName tname;
                    /* 创建类型名节点 */
                    CREATE_AST_NODE (tname, TypedefName);
                    /* typedef 的名字 */
                    tname->id   = TokenValue.p;
                    *tsTail     = (AstNode)tname;
                    tsTail      = &tname->next;
                    NEXT_TOKEN;
                    seeTy = 1;
                    break;
                }

                /* 如过是非类型名则直接结束 */
                return specs; 
            } 

            /* struct 或union 类型 */
            case TK_STRUCT: case TK_UNION: {

                *tsTail = (AstNode)ParseStructOrUnionSpecifier ();
                tsTail = &(*tsTail)->next;
                seeTy = 1;
            }break;

            /* 枚举类型 */
            case TK_ENUM: {
            
                *tsTail = (AstNode)ParseEnumSpecifier ();
                tsTail = &(*tsTail)->next;
                seeTy = 1;
            }break;

            /* 其他标示符直接返回 */
            default:
                return specs;
        }
    }
}

/* 类型的解析函数 */
/*
 * type-name:
 *      specifier-qualifier-list abstract-declarator
 * */
AstTypeName ParseTypeName (void)
{
    AstTypeName tyName;
    CREATE_AST_NODE (tyName, TypeName);

    /* 第一部分 */
    tyName->specs = ParseDeclarationSpecifiers ();
    if (tyName->specs->stgClasses) {
    
        Error (&tyName->coord, "type name should not have storage class");
        tyName->specs->stgClasses = NULL;
    }
    /* 第二部分 */
    tyName->dec = ParseDeclarator (DEC_ABSTRACT);
    return tyName;
}

/* 判断是否是类型 */
int IsTypeName (int tok) 
{
    /* 判断是否为typedef 类型别名,
     * 和基本类型复合类型 */
    return tok == TK_ID ? IsTypedefName (TokenValue.p) : 
        (tok >= TK_AUTO && tok <= TK_VOID);
} 

/* 定义的第二部分和初始化 */
/*
 * init-declarator:
 *      declarator
 *      declarator = initializer
 * */
static AstInitDeclarator ParseInitDeclarator (void)
{
    AstInitDeclarator initDec;

    CREATE_AST_NODE (initDec, InitDeclarator);
    /* 定义的第二部分和初始化 */
    initDec->dec = ParseDeclarator (DEC_CONCRETE);
    if (TK_ASSIGN == CurrentToken) {
        
        NEXT_TOKEN;
        /* 初始化模块 */
        initDec->init = ParseInitializer ();
    }
    return initDec;
}

/* 如果initDec 是一个合法的函数描述, 则返回函数描述的第二部分 */
static AstFunctionDeclarator GetFunctionDeclarator (AstInitDeclarator initDec) 
{
    AstDeclarator dec;

    /* 函数列表没有下一个, 和初始化模块 */
    if (!initDec || initDec->next || initDec->init)
        return NULL;

    /* 定义部分 */
    dec = initDec->dec;
    while (dec && NK_FunctionDeclarator != dec->kind) 
        dec = dec->dec;

    if (!dec || NK_NameDeclarator != dec->dec->kind) 
        return NULL;

    return (AstFunctionDeclarator)dec;
}

/* 完整声明(定义, 函数定义) */
static AstDeclaration ParseCommonHeader (void)
{
    AstDeclaration  decl;
    AstNode         *tail;

    CREATE_AST_NODE (decl, Declaration);

    /* 第一部分 */
    decl->specs = ParseDeclarationSpecifiers ();

    /* 如果是分号,则一个定义完成 */
    if (TK_SEMICOLON != CurrentToken) {

        /* 定义的第二部分和初始化 */
        decl->initDecs = (AstNode)ParseInitDeclarator ();
        tail = &decl->initDecs->next;
        /* 逗号操作符 */
        while (TK_COMMA == CurrentToken) {
        
            NEXT_TOKEN;
            *tail = (AstNode)ParseInitDeclarator ();
            tail = &(*tail)->next;
        }
    }
    return decl;
}

/* 解析定义, 和函数 
 * 如果函数则进如函数体 */
/* 
 * external-declaration:        
 *      function-definition     (函数)
 *      declaration             (定义)
 *
 * function-definition:         (函数)
 *      第一部分                第二部分 
 *      declaration-specifiers declarator [declaration-list] compound-statement
 *
 *  declaration:    (定义)
 *      declaration-specifiers [init-declarator-list] ;
 *
 *  declaration-list:
 *      declaration
 *      declaration-list declaration
 * */
static AstNode ParseExternalDeclaration (void)
{
    AstDeclaration      decl    = NULL;
    AstInitDeclarator   initDec = NULL;
    AstFunctionDeclarator fdec;

    /* 完整声明(变量, 函数定义) */
    decl = ParseCommonHeader ();
    /* 初始化部分 */
    initDec = (AstInitDeclarator)decl->initDecs;
    /* 全局变量定义 */
    if (decl->specs->stgClasses && TK_TYPEDEF == ((AstToken)decl->specs->stgClasses)->token) 
        goto not_func;

    /* 如果是合法的函数描述符, 则返回函数描述 */
    fdec = GetFunctionDeclarator (initDec);
    if ( fdec ) {

        AstFunction func;
        AstNode     *tail;

        /* 是分号(表示函数声明) */
        if (TK_SEMICOLON == CurrentToken) {

            NEXT_TOKEN;
            /* 函数声明, 不需要函数体 */
            if (TK_LBRACE != CurrentToken) 
                return (AstNode)decl;
            Error (&decl->coord, "maybe you accidently add the ;");
        } else if (fdec->paramTyList && TK_LBRACE != CurrentToken) {

            /* 没有参数列表并且,当前不是左大括号 */
            goto not_func;
        }

        /* 处理一个完整的函数 */
        /* 初始化一个函数 */
        CREATE_AST_NODE (func, Function);
        func->coord = decl->coord;
        func->specs = decl->specs;
        func->dec   = initDec->dec;
        func->fdec  = fdec;

        /* 处理函数参数 */
        Level++;
        /* 处理参数类型列表 */
        if ( func->fdec->paramTyList ) {
        
            /* 检查函数参数的类型 */
            AstNode p = func->fdec->paramTyList->paramDecls;
            for ( ; p; p = p->next) 
                CheckTypedefName (0, GetOutermostID (((AstParameterDeclaration)p)->dec));
            
        }

        tail = &func->decls;
        while (CurrentTokenIn (FIRST_Declaration)) {
            
            *tail = (AstNode)ParseDeclaration ();
            tail = &(*tail)->next;
        }
        Level--;

        /* 函数体定义下边的语句 */
        func->stmt = ParseCompoundStatement ();
        return (AstNode)func;
    }

not_func:
    Expect (TK_SEMICOLON);
    PreCheckTypedef (decl);

    return (AstNode)decl;
}

/* 语法分析的处理单元 */
/*
 * translation-unit:
 *      external-declaration
 *      translation-unit external-declaration
 * */
AstTranslationUnit ParseTranslationUnit (char *filename)
{
    AstTranslationUnit transUnit;
    AstNode *tail;

    /* 读取源文件,到内存数组中 */
    ReadSourceFile (filename);

    /* 初始化 TokenCoord */
    TokenCoord.filename = filename;
    TokenCoord.line = TokenCoord.col = TokenCoord.ppline = 1;

    /* 初始化两个集合 */
    TypedefNames    = CreateVector (8);
    OverloadNames   = CreateVector (8);

    /* 初始化一个常量0 */
    StructConstant0 ();

    CREATE_AST_NODE (transUnit, TranslationUnit);
    tail = &transUnit->extDecls;

    /* 开始取词 */
    NEXT_TOKEN;
    while (TK_END != CurrentToken) {
    
        if (TK_IMPORT == CurrentToken) {
            
            Import ();
            NEXT_TOKEN;
            continue;
        }

        /* 定义, 或者函数 */
        *tail = ParseExternalDeclaration ();
        tail = &(*tail)->next;
        SkipTo (FIRST_ExternalDeclaration, "the beginning of external declaration");
    }
    CloseSourceFile ();

    return transUnit;
}

/* 定义, 函数声明 */
AstDeclaration ParseDeclaration (void) 
{
    AstDeclaration decl;
    
    /* 定义, 函数头部 */
    decl = ParseCommonHeader ();
    /* 期望分号(声明) */
    Expect (TK_SEMICOLON);
    /* 检查定义 */
    PreCheckTypedef (decl);

    return decl;
}
