/*************************************************************************
	> File Name: expr.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月20日 星期四 18时19分36秒

	> Description: 表达式的语法分析
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "expr.h"

/* 各种操作符的优先级 */
static int Prec[] = {

    #define OPINFO(op, prec, name, func, opcode) prec, 
    #include "opinfo.h"
    0
    #undef OPINFO
};

/* 常量0 表达式 */
AstExpression Constant0;

/* 操作的名字 */
char *OPNames[] = {

    #define OPINFO(op, prec, name, func, opcode) name, 
    #include "opinfo.h"
        NULL
    #undef OPINFO
};

/* 各种操作符的一元二元操作的含义 */
static struct tokenOp TokenOps[] = {

    /* 使用到的bop,uop 已经在.h文件枚举列出 */
    #define TOKENOP(tok, bop, uop) {bop, uop},
    #include "tokenop.h"
    #undef  TOKENOP
};

/* 构造常量0 */
void StructConstant0 (void)
{
    if (Constant0) return;

    CREATE_AST_NODE (Constant0, Expression);
    Constant0->ty    = T(INT);
    Constant0->op    = OP_CONST;
    Constant0->val.i[0] = 0;
}

/* 基本表达式 */
/*
 * primary-expression:      (基本表达式)
 *      ID                  (变量)
 *      constant            (常量)
 *      string-literal      (字符串)
 *      (expression)        (表达式)
 * */
static AstExpression ParsePrimaryExpression (void) 
{
    AstExpression expr;

    switch (CurrentToken) {

        /* 变量 */
        case TK_ID: {

            /* 检查表达式的id 在使用之前是否已经定义 */
            /*
            printf ("TK_ID: %s \n", (char *)TokenValue.p);
            if (NULL == LookupID (TokenValue.p)) {

                Error (&TokenCoord, "Undeclared identifier: %s", TokenValue.p);
            } */

            CREATE_AST_NODE (expr, Expression);
            expr->op    = OP_ID;
            expr->val   = TokenValue;
            NEXT_TOKEN;

            return expr;
        }

        /* 表达式是常量 */
        case TK_INTCONST:   case TK_UINTCONST:  case TK_LONGCONST:
        case TK_ULONGCONST: case TK_LLONGCONST: case TK_ULLONGCONST:
        case TK_FLOATCONST: case TK_DOUBLECONST:case TK_LDOUBLECONST: {
        
            CREATE_AST_NODE (expr, Expression);
            /* 浮点常量的类型提升 */
            if (TK_FLOATCONST <= CurrentToken) 
                CurrentToken++;

            expr->ty    = T(INT + CurrentToken - TK_INTCONST);
            expr->op    = OP_CONST;
            expr->val   = TokenValue;
            NEXT_TOKEN;
            return expr;
        }

        /* 字符串处理 */
        case TK_STRING: case TK_WIDESTRING: {

            CREATE_AST_NODE (expr, Expression);
            /* 创建一个数组 */
            expr->ty = ArrayOf (((String)TokenValue.p)->len + 1, 
                    CurrentToken == TK_STRING ? T (CHAR) : WCharType);
            expr->op    = OP_STR;
            expr->val   = TokenValue;
            NEXT_TOKEN;
            return expr;
        }

        /* 小括号里的表达式 */
        case TK_LPAREN: {
    
            /* 跳过小左括号 */
            NEXT_TOKEN;
            /* 分析括号里的表达式 */
            expr = ParseExpression ();
            /* 表达式分析完,期望右小括号 */
            Expect (TK_RPAREN);

            return expr;
        }

        default:
            /* 此处期望基本表达式 */
            Error(&TokenCoord, "Expect identifier, string, constant or (");
            return Constant0;
    }
}

/* 后缀表达式 */
/* 
 * postfix-expression:                      (后缀表达式)
 *      primary-expression                  (基本表达式)
 *      postfix-expression [expression]     (数组)
 *      postfix-expression ([argument-expression-list]) (函数调用)
 *      postfix-expression . identifier     (. 成员)
 *      postfix-expression -> identifier    (->成员)
 *      postfix-expression ++               (滞后自增)
 *      postfix-expression --               (滞后自减)
 *      postfix-expression
 * */
static AstExpression ParsePostfixExpression (void)
{
    AstExpression expr, p;

    /* 基本表达式 */
    expr = ParsePrimaryExpression ();

    while (1) {
    
        switch (CurrentToken) {

            /* 下标操作 (前边的基本表达式为第一个操作数,
             * 中括号里的表达式为第二个操作数) */
            case TK_LBRACKET: {
    
                CREATE_AST_NODE (p, Expression);
                p->op       = OP_INDEX;
                p->kids[0]  = expr;
                NEXT_TOKEN;
                p->kids[1] = ParseExpression ();
                Expect (TK_RBRACKET);
                expr = p;
            } break;

            /* 函数调用 (前边的基本表达式为第一个操作数, 参数是第二个操作数) */
            case TK_LPAREN: {

                CREATE_AST_NODE (p, Expression);
                p->op       = OP_CALL;
                p->kids[0]  = expr;
                NEXT_TOKEN;

                /* 扫描参数列表 */
                if (TK_RPAREN != CurrentToken) {

                    AstNode     *tail;

                    /* 实参向形参的备份过程 */
                    p->kids[1]  = ParseAssignmentExpression ();
                    tail        = &p->kids[1]->next;
                    while (TK_COMMA == CurrentToken) {
                        
                        NEXT_TOKEN;
                        *tail   = (AstNode)ParseAssignmentExpression ();
                        tail    = &(*tail)->next;
                    }
                }
                Expect (TK_RPAREN);
                expr = p;
            }break;

            /* 成员访问 (前边的基本表达式为第一个操作数, 成员是他的值) */
            case TK_DOT: case TK_POINTER: {
    
                CREATE_AST_NODE (p, Expression);
                /* 设置操作符号 . 或者 -> */
                p->op = (TK_DOT == CurrentToken ? OP_MEMBER : OP_PTR_MEMBER);
                p->kids[0] = expr;
                NEXT_TOKEN;
                if (TK_ID != CurrentToken) {
    
                    Error(&p->coord, "Expect identifier as struct or union member");
                } else {

                    p->val = TokenValue;
                    NEXT_TOKEN;
                }

                expr = p;
            }break;

            /* 滞后自增自减 */
            case TK_INC: case TK_DEC: {
    
                CREATE_AST_NODE (p, Expression);
                /* 设置++ 或者 -- 操作 */
                p->op = (TK_INC == CurrentToken ? OP_POSTINC : OP_POSTDEC);
                /* 滞后++ (--) */
                p->kids[0] = expr;
                NEXT_TOKEN;
                expr = p;
            }break;

            default: return expr;
        }
    }
}

/* 一元表达式 */
/*
 * unary-expression:                    (一元表达式)
 *      postfix-expression              (后缀表达式)
 *      unary-operator unary-expression (一元操作符 一元表达式)
 *      (type-name) unary-expression    (强制类型转换)
 *      sizeof unary-expression         (sizeof 一元表达式)
 *      sizeof (type-name)              (sizeof (类型名))
 *
 * unary-operator:                      (一元操作符)
 *      ++ -- & * + - ! ~
 * */
static AstExpression ParseUnaryExpression (void) 
{
    AstExpression expr;
    int t;

    switch (CurrentToken) {

        /* 一元操作符(操作符后边的表达式为操作符的第一个操作数) */
        case TK_INC: case TK_DEC: case TK_BITAND: case TK_MUL:
        case TK_ADD: case TK_SUB: case TK_NOT: case TK_COMP: {
            
            /* 创建一个表达式节点 */
            CREATE_AST_NODE (expr, Expression);
            /* 取得一元操作符的token */
            expr->op = UNARY_OP;
            NEXT_TOKEN;
            expr->kids[0] = ParseUnaryExpression ();
            return expr;
        }

        /* 强制类型转换 (类型为第一个操作数,后边的一元表达式为第二个操作数) */
        case TK_LPAREN: {
    
            /* 一个范围的开始 */
            BeginPeekToken ();
            /* 没有修改 CurrentToken */
            t = GetNextToken ();
            /* 强制类型转换 */
            if (IsTypeName (t)) {
                
                EndPeekToken ();
                CREATE_AST_NODE (expr, Expression);
                /* 强制类型转换 */
                expr->op = OP_CAST;
                NEXT_TOKEN;
                /* 取得强制类型转换的类型 */
                expr->kids[0] = (AstExpression)ParseTypeName ();
                /* 期望下一个右小括号 */
                Expect (TK_RPAREN);
                /* 取得强制类型转换的操作数 */
                expr->kids[1] = ParseUnaryExpression ();
                return expr;
            } else {

                EndPeekToken ();
                /* 非强制类型转换,则是后缀表达式 */
                return ParsePostfixExpression ();
            } 
        }break; 

        case TK_SIZEOF: {

            CREATE_AST_NODE (expr, Expression);
            expr->op = OP_SIZEOF;
            NEXT_TOKEN;

            /* sizeof 带有括号 */
            if (TK_LPAREN == CurrentToken) {

                BeginPeekToken ();
                t = GetNextToken ();
                /* 对类型做sizeof 操作时,必须带括号 */
                if (IsTypeName (t)) {

                    EndPeekToken ();
                    NEXT_TOKEN;
                    /* 操作数是类型 */
                    expr->kids[0] = (AstExpression)ParseTypeName ();
                    Expect (TK_RPAREN);
                } else {
    
                    EndPeekToken ();
                    /* sizeof 操作数非类型则是一元表达式 */
                    expr->kids[0] = ParseUnaryExpression ();
                }
            } else {

                /* 没有括号则必须是一元表达式 */
                expr->kids[0] = ParseUnaryExpression ();
            }
            return expr;
        }break;

        default: 
            return ParsePostfixExpression ();
    }
}

/* 二元表达式 (不包括赋值)  (包括一元)
 * 如果当前优先级高, 则继续向后扫描 */
static AstExpression ParseBinaryExpression (int prec)
{
    AstExpression binExpr;
    AstExpression expr;
    int newPrec;

    /* 一元表达式(二元表达式的第一元) */
    expr = ParseUnaryExpression ();

    /* 是双目操作符,当前符号优先级要高 */
    while (IsBinaryOP (CurrentToken) && (newPrec = Prec[BINARY_OP]) >= prec) {
    
        CREATE_AST_NODE (binExpr, Expression);
        binExpr->op = BINARY_OP;
        binExpr->kids[0] = expr;
        NEXT_TOKEN;
        binExpr->kids[1] = ParseBinaryExpression (newPrec + 1);

        expr = binExpr;
    }
    return expr;
}

/* 表达式 */
/*
 * conditional-expression       (表达式)
 *      logical-OR-expression   (逻辑表达式,算数表达式)
 *      (逻辑表达式,算数表达式  表达式       表达式)
 *      logical-OR-expression ? expression : conditional-expression (三目运算)
 * */
static AstExpression ParseConditionalExpression (void) 
{
    AstExpression expr;

    /* 二元操作 */
    expr = ParseBinaryExpression (Prec[OP_OR]);

    /* 三目运算符eg:
     * a > b ? c : d */
    if (TK_QUESTION == CurrentToken) {

        AstExpression condExpr;
        CREATE_AST_NODE (condExpr, Expression);
        condExpr->op = OP_QUESTION;
        /* 存储(a > b) */
        condExpr->kids[0] = expr;
        NEXT_TOKEN;

        CREATE_AST_NODE (condExpr->kids[1], Expression);
        /* 分号操作符 */
        condExpr->kids[1]->op = OP_COLON;
        /* 存储(c) */
        condExpr->kids[1]->kids[0] = ParseExpression ();
        Expect (TK_COLON);
        /* 存储(d) */
        condExpr->kids[1]->kids[1] = ParseConditionalExpression ();

        return condExpr;
    }
    return expr;
}


/* 赋值表达式 */
/* 
 * assignment-expression:       (赋值表达式)
 *      conditional-expression  (表达式)
 *      (一元表达式      赋值操作符          赋值表达式)
 *      unary-expression assignment-operator assignment-expression
 *
 *  assignment-operator:        (赋值操作符)
 *      = *= /= %= += -= <<= >>= &= ^= |= 
 * */
AstExpression ParseAssignmentExpression (void)
{
    AstExpression expr;

    /* 条件表达式 */
    expr = ParseConditionalExpression ();

    /* 赋值操作符 */
    if (TK_ASSIGN <= CurrentToken && TK_MOD_ASSIGN >= CurrentToken) {

        AstExpression asgnExpr;
        CREATE_AST_NODE (asgnExpr, Expression);

        asgnExpr->op        = BINARY_OP;
        asgnExpr->kids[0]   = expr;
        NEXT_TOKEN;
        asgnExpr->kids[1]   = ParseAssignmentExpression ();
        return asgnExpr;
    }
    return expr;
}

/* 常量表达式 */
AstExpression ParseConstantExpression (void) 
{
    /* 条件表达式 */
    return ParseConditionalExpression ();
}

/* 表达式 */
/*
 *  expression:                 (表达式)
 *      assignment-expression   (赋值表达式)
 *      expression , assignment-expression  (逗号表达式)
 */
AstExpression ParseExpression (void) 
{
    /* 表达式和逗号表达式 */
    AstExpression expr, comaExpr;

    /* 赋值表达式 */
    expr = ParseAssignmentExpression ();

    /* 逗号表达式 */
    while (TK_COMMA == CurrentToken) {
    
        CREATE_AST_NODE (comaExpr, Expression);

        comaExpr->op        = OP_COMMA;
        comaExpr->kids[0]   = expr;
        NEXT_TOKEN;
        comaExpr->kids[1]   = ParseAssignmentExpression ();

        expr = comaExpr;
    }
    return expr;
}
