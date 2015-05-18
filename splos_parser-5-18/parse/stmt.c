/*************************************************************************
	> File Name: stmt.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月30日 星期日 10时39分59秒

	> Description: c语言语句的语法分析
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "stmt.h"
#include "grammer.h"

/* 语句中出现的token */
static int FIRST_Statement[] = {FIRST_STATEMENT, 0};

/* 处理所有状态语句 */
static AstStatement ParseStatement (void);

/* 表达式语句 */
/* 
 * expression-statement:    (表达式语句)
 *      [expression]        (表达式)
 * */
static AstStatement ParseExpressionStatement (void)
{
    AstExpressionStatement exprStmt;
    CREATE_AST_NODE (exprStmt, ExpressionStatement);

    if (TK_SEMICOLON != CurrentToken) {
        
        /* 表达式 */
        exprStmt->expr = ParseExpression ();
    }
    /* 表达式语句后边的分号 */
    Expect (TK_SEMICOLON);
    return (AstStatement)exprStmt;
}

/* 标签语句, 或表达式 */
/*
 * label-statement:     (标签语句)
 *      ID : statement  (标签和后边的语句)
 * */
static AstStatement ParseLabelStatement (void)
{
    AstLabelStatement labelStmt;
    int t;

    BeginPeekToken ();
    t = GetNextToken ();
    
    if (TK_COLON == t) {
    
        /* 标签名后边是冒号 */
        EndPeekToken ();
        CREATE_AST_NODE (labelStmt, LabelStatement);
        labelStmt->id = TokenValue.p;
        /* 跳过标签名和冒号 */
        NEXT_TOKEN;
        NEXT_TOKEN;
        /* 处理标签后边的复合语句 */
        labelStmt->stmt = ParseStatement ();
        return (AstStatement)labelStmt;
    } else {
    
        /* 表达式 */
        EndPeekToken ();
        return ParseExpressionStatement ();
    }
}

/* case 语句 */
/* 
 * case-statement:
 *      case constant-expression : statement
 * */
static AstStatement ParseCaseStatement (void)
{
    AstCaseStatement caseStmt;
    CREATE_AST_NODE (caseStmt, CaseStatement);

    NEXT_TOKEN;
    /* 处理case 后边的常量 */
    caseStmt->expr = ParseConstantExpression ();
    Expect (TK_COLON);
    /* caes 后边的复合语句 */
    caseStmt->stmt = ParseStatement ();

    return (AstStatement)caseStmt;
}

/* default 语句 */ 
/*
 * default-statement:
 *      default : statement
 * */
static AstStatement ParseDefaultStatement (void)
{
    AstDefaultStatement defStmt;
    CREATE_AST_NODE (defStmt, DefaultStatement);

    NEXT_TOKEN;
    Expect (TK_COLON);
    /* 处理default 后边的模块 */
    defStmt->stmt = ParseStatement ();

    return (AstStatement)defStmt;
}

/* if 语句 */
/*
 * if-statement:                    (if 语句)
 *      if (expression) statement   (if 条件 语句)
 *      if (expression) statement else statement    (if else)
 * */
static AstStatement ParseIfStatement (void)
{
    AstIfStatement ifStmt;
    CREATE_AST_NODE (ifStmt, IfStatement);

    NEXT_TOKEN;
    Expect (TK_LPAREN);
    /* if 条件 */
    ifStmt->expr = ParseExpression ();
    Expect (TK_RPAREN);
    
    /* if then 语句 */
    ifStmt->thenStmt = ParseStatement ();

    /* else 模块 */
    if (TK_ELSE == CurrentToken) {

        NEXT_TOKEN;

        ifStmt->elseStmt = ParseStatement ();
    }
    return (AstStatement)ifStmt;
}

/* switch 语句 */
/* 
 * switch-statement:
 *      switch (expression) statement
 * */
static AstStatement ParseSwitchStatement (void) 
{
    AstSwitchStatement swtchStmt;
    CREATE_AST_NODE (swtchStmt, SwitchStatement);

    NEXT_TOKEN;
    Expect (TK_LPAREN);
    /* switch 的表达式 */
    swtchStmt->expr = ParseExpression ();
    Expect (TK_RPAREN);
    /* switch 后边的模块 */
    swtchStmt->stmt = ParseStatement ();
    return (AstStatement)swtchStmt;
}

/* 处理while 模块 */
/*
 * while-statement:
 *      while (expression) statement
 * */
static AstStatement ParseWhileStatement (void)
{
    AstLoopStatement whileStmt;
    CREATE_AST_NODE (whileStmt, WhileStatement);

    NEXT_TOKEN;
    Expect (TK_LPAREN);
    /* while 条件 */
    whileStmt->expr = ParseExpression ();
    Expect (TK_RPAREN);
    /* while 循环体 */
    whileStmt->stmt = ParseStatement ();

    return (AstStatement)whileStmt;
}

/* 描述do 的结构 */
/* 
 * do-statement:
 *      do statement while (expression) ;
 * */
static AstStatement ParseDoStatement ()
{
    AstLoopStatement doStmt;
    CREATE_AST_NODE (doStmt, DoStatement);

    NEXT_TOKEN;
    /* do 循环体 */
    doStmt->stmt = ParseStatement ();
    /* do的循环体外边必须是while */
    Expect (TK_WHILE);
    Expect (TK_LPAREN);
    /* while 条件 */
    doStmt->expr= ParseExpression ();
    Expect (TK_RPAREN);
    /* while 后边的分号 */
    Expect (TK_SEMICOLON);

    return (AstStatement)doStmt;
}

/* 处理for 循环 */
/*
 * for-statement:
 * statementfor ( [expression] ; [expression] ; [expression]  ) statement
 * */
static AstStatement ParseForStatement ()
{
    AstForStatement forStmt;
    CREATE_AST_NODE (forStmt, ForStatement);

    NEXT_TOKEN;    
    Expect (TK_LPAREN);
    
    if (TK_SEMICOLON != CurrentToken) {
        /* 初始条件 */
        forStmt->initExpr = ParseExpression ();
    }
    Expect (TK_SEMICOLON);
    
    if (TK_SEMICOLON != CurrentToken) {
        /* for 条件 */
        forStmt->expr = ParseExpression ();
    }
    Expect(TK_SEMICOLON);

    if (TK_RPAREN != CurrentToken) {
        /* for 步长 */
        forStmt->incrExpr = ParseExpression ();
    }
    Expect(TK_RPAREN);
    
    /* for 的循环体 */
    forStmt->stmt = ParseStatement ();
    return (AstStatement)forStmt;    
}

/* 处理goto 操作 */
/*
 * goto-statement:
 *		goto ID ;
 */
static AstStatement ParseGotoStatement(void)
{
	AstGotoStatement gotoStmt;
	CREATE_AST_NODE (gotoStmt, GotoStatement);

	NEXT_TOKEN;
	if (TK_ID == CurrentToken) {

        /* 要跳转到的label 的名字 */
		gotoStmt->id = TokenValue.p;
		NEXT_TOKEN;
		Expect (TK_SEMICOLON);
	} else {

		Error(&TokenCoord, "Expect identifier");
		if (CurrentToken == TK_SEMICOLON)
			NEXT_TOKEN;
	}
	return (AstStatement)gotoStmt;
}

/* 描述break 操作 */
/*
 * break-statement:
 *		break ;
 */
static AstStatement ParseBreakStatement (void)
{
	AstBreakStatement brkStmt;
	CREATE_AST_NODE (brkStmt, BreakStatement);

	NEXT_TOKEN;
	Expect (TK_SEMICOLON);
	return (AstStatement)brkStmt;
}

/* 描述continue 操作 */
/*
 * continue-statement:
 *		continue ;
 */
static AstStatement ParseContinueStatement (void)
{
	AstContinueStatement contStmt;
	CREATE_AST_NODE (contStmt, ContinueStatement);

	NEXT_TOKEN;
	Expect (TK_SEMICOLON);
	return (AstStatement)contStmt;
}

/* 描述return 操作 */
/*
 * return-statement:
 *		return [expression] ;
 */
static AstStatement ParseReturnStatement (void)
{
	AstReturnStatement retStmt;
	CREATE_AST_NODE (retStmt, ReturnStatement);

	NEXT_TOKEN;
	if (CurrentToken != TK_SEMICOLON) {

        /* 返回值表达式 */
		retStmt->expr = ParseExpression ();
	}
	Expect(TK_SEMICOLON);

	return (AstStatement)retStmt;
}

/* 描述并行语句 
 * 
 * parallel-statement: 
 *      parallel {statement} 
 */
static AstStatement ParseParallelStatement (void)
{
    AstParallelStatement    parStmt;

    CREATE_AST_NODE (parStmt, ParallelStatement);

    NEXT_TOKEN;
    if (CurrentToken == TK_LBRACE) {

        parStmt->stmt = ParseCompoundStatement ();
    } else Expect (TK_LBRACE);

    return (AstStatement)parStmt;
}

/* 所有语句状态 */
/*
 * statement:                   (语句)
 *      expression-statement    (表达式语句)
 *      labeled-statement       (标签语句)
 *      case-statement          (case 语句)
 *      default-statement       (default 语句)
 *      if-statement            (if 语句)
 *      switch-statement        (switch 语句)
 *      while-statement         (while 语句)
 *      do-statement            (do 语句)
 *      for-statement           (for 语句)
 *      goto-statement          (goto 语句)
 *      continue-statement      (continue 语句)
 *      break-statement         (break 语句)
 *      return-statement        (return 语句)
 *      parallel-statement      (parallel 并行语句)
 *      compound-statement      (复合语句)
 * */
static AstStatement ParseStatement (void) 
{
    switch (CurrentToken) {

        /* 可能是标签,也可能是表达式 */
        case TK_ID:
            return ParseLabelStatement (); 

        case TK_CASE:
            return ParseCaseStatement ();

        case TK_DEFAULT:
            return ParseDefaultStatement (); 

        case TK_IF:
            return ParseIfStatement ();
        
        case TK_SWITCH:
            return ParseSwitchStatement (); 

        case TK_WHILE:
            return ParseWhileStatement ();

        case TK_DO:
            return ParseDoStatement ();

        case TK_FOR:
            return ParseForStatement ();

        case TK_GOTO:
            return ParseGotoStatement ();

        case TK_CONTINUE:
            return ParseContinueStatement ();

        case TK_BREAK:
            return ParseBreakStatement ();

        case TK_RETURN:
            return ParseReturnStatement ();

        /* 并行语句 */
        case TK_PARALLEL:
            return ParseParallelStatement ();

        case TK_LBRACE:
            return ParseCompoundStatement (); 

        default:
            return ParseExpressionStatement ();
    }
}

/* 复合语句(一个模块)
 * 第一步: 扫描变量定义列表
 * 第二部: 扫描语句 */
/* 
 * compound-statement:                              (一个模块)
 *      { [declaration-list] [statement-list] }     (一个模块(语句列表))
 *
 * declaration-lsit:                    (变量的定义列表)
 *      declaration                     (变量的定义)
 *      declaration-list declaration
 * statement-list:                      (语句列表)
 *      statement                       (语句)
 *      statement-list statement
 * */
#if 1
AstStatement ParseCompoundStatement (void)
{
    AstCompoundStatement compStmt;
    AstNode *tail;

    /* 进入大括号内部 */
    Level++;
    CREATE_AST_NODE (compStmt, CompoundStatement);

    NEXT_TOKEN;

    /* 变量的定义列表 */
    tail = &compStmt->decls;
    while (CurrentTokenIn (FIRST_Declaration)) {
    
        /* 开头不是类型则变量定义结束 */
        if (TK_ID == CurrentToken && !IsTypeName (CurrentToken)) 
            break;

        *tail = (AstNode)ParseDeclaration ();
        tail = &(*tail)->next;
    }

    /* 语句定义列表 */
    tail = &compStmt->stmts;
    while (TK_RBRACE != CurrentToken && TK_END != CurrentToken) {
    
        /* 语句的处理 */
        *tail = (AstNode)ParseStatement ();
        tail = &(*tail)->next;

        /* 遇到右大括号则结束 */
        if (TK_RBRACE == CurrentToken)
            break;
        SkipTo (FIRST_Statement, "the beginning of a statement");
    }
    Expect (TK_RBRACE);

    /* 一个模块结束,去除所有重载 */
    PostCheckTypedef ();
    Level--;
    return (AstStatement)compStmt;
}

#elif 0
AstStatement ParseCompoundStatement (void)
{
    AstCompoundStatement compStmt;
    AstNode *declTail;
    AstNode *stmtTail;

    /* 进入大括号内部 */
    Level++;
    CREATE_AST_NODE (compStmt, CompoundStatement);

    NEXT_TOKEN;

    /* 变量的定义列表 */
    declTail = &compStmt->decls;
    stmtTail = &compStmt->stmts;

    /* 语句定义列表 */
    while (TK_RBRACE != CurrentToken && TK_END != CurrentToken) {
    
        if (CurrentTokenIn (FIRST_Declaration)) {

            if (TK_ID != CurrentToken || IsTypeName (CurrentToken)) {

                *declTail = (AstNode)ParseDeclaration ();
                declTail = &(*declTail)->next;
                continue;
            }
        }

        /* 语句的处理 */
        *stmtTail = (AstNode)ParseStatement ();
        stmtTail = &(*stmtTail)->next;

        /* 遇到右大括号则结束 */
        if (TK_RBRACE == CurrentToken)
            break;
        SkipTo (FIRST_Statement, "the beginning of a statement");
    }
    Expect (TK_RBRACE);

    /* 一个模块结束,去除所有重载 */
    PostCheckTypedef ();
    Level--;
    return (AstStatement)compStmt;
}
#endif
