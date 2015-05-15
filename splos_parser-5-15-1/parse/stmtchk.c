/*************************************************************************
	> File Name: stmtchk.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月20日 星期六 18时20分38秒

	> Description: 
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "stmt.h"
#include "decl.h"
#include "expr.h"

/* 向集合中添加状态 */
#define PushStatement(v, stmt)   INSERT_ITEM(v, stmt)
#define PopStatement(v)          (v->data[--v->len])
/* 取集合中的状态 */
#define TopStatement(v)          TOP_ITEM(v)

/* 检查一个模块的语义 */
static AstStatement CheckStatement (AstStatement stmt);

/* 尝试添加标签 */
static Label TryAddLabel (char *id)
{
	Label p = CURRENTF->labels;

    /* 在函数内部查找标号 */
	for ( ; p; p = p->next) 
		if (p->id == id)
			return p;

    /* 若没有，则添加 */
	CALLOC (p);
	p->id = id;
	p->next = CURRENTF->labels;
	CURRENTF->labels = p;

	return p;
}


static void AddCase (AstSwitchStatement swtchStmt, AstCaseStatement caseStmt)
{
    /* switch 的case语句 */
	AstCaseStatement p = swtchStmt->cases;
	AstCaseStatement *pprev = &swtchStmt->cases;
	int diff;

    /* 检查case 是否在switch 中已经定义过 */
	while (p) {

		diff = caseStmt->expr->val.i[0] - p->expr->val.i[0];
		if (diff < 0)
			break;

		if (diff > 0) {

			pprev = &p->nextCase;
			p = p->nextCase;
		} else {
            /* 重定义 */
			Error(&caseStmt->coord, "Repeated constant in a switch statement");
			return;
		}
	}

    /* 没有定义则添加 */
	*pprev = caseStmt;
	caseStmt->nextCase = p;
}

/* 检查表达式 */
static AstStatement CheckExpressionStatement (AstStatement stmt)
{
	AstExpressionStatement exprStmt = AsExpr (stmt);

	if (exprStmt->expr != NULL) {

		exprStmt->expr = CheckExpression (exprStmt->expr);
	}
	return stmt;
}

/* 检查函数内部的标签 */
static AstStatement CheckLabelStatement (AstStatement stmt)
{
	AstLabelStatement labelStmt = AsLabel (stmt);

    /* 添加标签 */
	labelStmt->label = TryAddLabel (labelStmt->id);
	if (labelStmt->label->defined) {

		Error(&stmt->coord, "Label name should be unique within function.");
	}
	labelStmt->label->defined   = 1;
	labelStmt->label->coord     = stmt->coord;
	labelStmt->stmt = CheckStatement (labelStmt->stmt);

	return stmt;
}

/* case语义检查 */
static AstStatement CheckCaseStatement(AstStatement stmt)
{
	AstCaseStatement    caseStmt = AsCase (stmt);
	AstSwitchStatement  swtchStmt;

	swtchStmt = (AstSwitchStatement)TopStatement (CURRENTF->swtches);
	if (swtchStmt == NULL) {

		Error(&stmt->coord, "A case label shall appear in a switch statement.");
		return stmt;
	}

    /* 检查case后的表达式 */
	caseStmt->expr = CheckConstantExpression (caseStmt->expr);
	if (caseStmt->expr == NULL) {
    
        /* case之后必须要有表达式 */
		Error (&stmt->coord, "The case value must be integer constant.");
//		return stmt;
	}

    /* 检查case后的模块 */
	caseStmt->stmt = CheckStatement (caseStmt->stmt);
    /* case 之后的表达式和switch的条件的类型转换 */
    caseStmt->expr = caseStmt->expr ? FoldCast (swtchStmt->expr->ty, caseStmt->expr) : NULL;
    /* 添加case switch语句中 */
	AddCase (swtchStmt, caseStmt);

	return stmt;
}

/* default模块 */
static AstStatement CheckDefaultStatement (AstStatement stmt)
{
	AstDefaultStatement defStmt = AsDef (stmt);
	AstSwitchStatement  swtchStmt;

    /* 取得switch 语句 */
	swtchStmt = (AstSwitchStatement)TopStatement (CURRENTF->swtches);
	if (swtchStmt == NULL) {

		Error(&stmt->coord, "A default label shall appear in a switch statement.");
		return stmt;
	}

    /* default 模块在一个switch 中只能有一个 */
	if (swtchStmt->defStmt != NULL) {

		Error(&stmt->coord, "There shall be only one default label in a switch statement.");
		return stmt;
	}

    /* 检查default 语句状态并添加 */
	defStmt->stmt = CheckStatement (defStmt->stmt);
	swtchStmt->defStmt = defStmt;

	return stmt;
}

/* 检查if的语义 */
static AstStatement CheckIfStatement (AstStatement stmt)
{
	AstIfStatement ifStmt = AsIf (stmt);

    /* 检查if 条件 */
	ifStmt->expr = Adjust (CheckExpression (ifStmt->expr), 1);
	if (!ifStmt->expr) {

        Error (&stmt->coord, "The if must have expression.");
    } else if (!IsScalarType (ifStmt->expr->ty)) {

		Error (&stmt->coord, "The expression in if statement shall be scalar type.");
	}
    /* 检查if then 的语句 */
	ifStmt->thenStmt = CheckStatement (ifStmt->thenStmt);
    /* 如果有else 检查else语句 */
	if (ifStmt->elseStmt) {

		ifStmt->elseStmt = CheckStatement (ifStmt->elseStmt);
	}
	return stmt;
}

/* switch语句 */
static AstStatement CheckSwitchStatement (AstStatement stmt)
{
	AstSwitchStatement swtchStmt = AsSwitch (stmt);

    /* case 和break 状态 */
	PushStatement (CURRENTF->swtches,   stmt);
	PushStatement (CURRENTF->breakable, stmt);

    /* 检查条件 */
	swtchStmt->expr = Adjust (CheckExpression (swtchStmt->expr), 1);
	if (!IsIntegType (swtchStmt->expr->ty)) {

        /* 条件的结构必须是整形 */
		Error(&stmt->coord, "The expression in a switch statement shall be integer type.");
		swtchStmt->expr->ty = T(INT);
	}

	if (swtchStmt->expr->ty->categ < INT) {

        /* 提升条件类型的等级 */
		swtchStmt->expr = Cast(T(INT), swtchStmt->expr);
	}
    /* 检查switch 的内部模块 */
	swtchStmt->stmt = CheckStatement (swtchStmt->stmt);

    /* 退出switch 模块 */
	PopStatement (CURRENTF->swtches);
	PopStatement (CURRENTF->breakable);

	return stmt;
}

/* while循环 */
static AstStatement CheckLoopStatement (AstStatement stmt)
{
	AstLoopStatement loopStmt = AsLoop (stmt);

	PushStatement (CURRENTF->loops,    stmt);
	PushStatement (CURRENTF->breakable, stmt);

    /* 检查条件 */
	loopStmt->expr = Adjust (CheckExpression (loopStmt->expr), 1);
	if (! IsScalarType(loopStmt->expr->ty)) {

		Error(&stmt->coord, "The expression in do or while statement shall be scalar type.");
	}

    /* 检查循环体 */
	loopStmt->stmt = CheckStatement(loopStmt->stmt);
	/* 退出循环 */
	PopStatement(CURRENTF->loops);
	PopStatement(CURRENTF->breakable);

	return stmt;
}


/* for 的语句 */
static AstStatement CheckForStatement(AstStatement stmt)
{
	AstForStatement forStmt = AsFor (stmt);

    /* 循环添加循环状态，和break */
	PushStatement (CURRENTF->loops,     stmt);
	PushStatement (CURRENTF->breakable, stmt);

    /* for 的初始化表达式 */
	if (forStmt->initExpr) {

		forStmt->initExpr = CheckExpression (forStmt->initExpr);
	}
    /* for的条件 */
	if (forStmt->expr) {

		forStmt->expr = Adjust (CheckExpression (forStmt->expr), 1);
		if (!IsScalarType (forStmt->expr->ty)) {

			Error(&stmt->coord, "The second expression in for statement shall be scalar type.");
		}
	}
    /* for的步长 */
	if (forStmt->incrExpr) {

		forStmt->incrExpr = CheckExpression (forStmt->incrExpr);
	}

    /* 检查循环体 */
	forStmt->stmt = CheckStatement (forStmt->stmt);
    /* 循环结束退出 */
	PopStatement (CURRENTF->loops);
	PopStatement (CURRENTF->breakable);

	return stmt;
}

/* goto 语义检查 */
static AstStatement CheckGotoStatement(AstStatement stmt)
{
	AstGotoStatement gotoStmt = AsGoto (stmt);

    /* id 是goto 的标号 */
	if (gotoStmt->id != NULL) {

        /* 查找标号，若没有则添加 */
		gotoStmt->label         = TryAddLabel (gotoStmt->id);
		gotoStmt->label->coord  = gotoStmt->coord;
		gotoStmt->label->ref++;
	}
	return stmt;
}

/* break 语义检查 */
static AstStatement CheckBreakStatement(AstStatement stmt)
{
	AstBreakStatement brkStmt = AsBreak (stmt);

    /* 栈顶的break 跳出点 */
	brkStmt->target = TopStatement (CURRENTF->breakable);
	if (brkStmt->target == NULL) {

		Error(&stmt->coord, "The break shall appear in a switch or loop");
	}

	return stmt;
}

/* continue 语义检查 */
static AstStatement CheckContinueStatement(AstStatement stmt)
{
	AstContinueStatement contStmt = AsCont (stmt);

    /* 取一层循环 */
	contStmt->target = (AstLoopStatement)TopStatement (CURRENTF->loops);
    /* continue 必须要有目标 */
	if (contStmt->target == NULL) {

		Error(&stmt->coord, "The continue shall appear in a loop.");
	}
	return stmt;
}

/* return 的语义检查 */
static AstStatement CheckReturnStatement(AstStatement stmt)
{
	AstReturnStatement retStmt = AsRet (stmt);
    /* 返回值的类型 */
	Type rty = FSYM->ty->bty;

    /* 设置当前所在函数有返回值 */
	CURRENTF->hasReturn = 1;
	if (retStmt->expr) {

        /* 检查调整返回值的状态 */
		retStmt->expr = Adjust (CheckExpression(retStmt->expr), 1);
		if (rty->categ == VOID) {

            /* 该函数没有返回值 */
			Error (&stmt->coord, "Void function should not return value");
			return stmt;
		}

		if (!CanAssign (rty, retStmt->expr)) {

            /* 表达式,返回值类型不兼容 */
			Error(&stmt->coord, "Incompatible return value");
			return stmt;
		}
		retStmt->expr = Cast (rty, retStmt->expr);
		return stmt;
	}

	if (rty->categ != VOID) {

        /* 函数应该有返回值 */
		Warning (&stmt->coord, "The function should return a value.");
	}
	return stmt;
}

/* 一个模块的语义检查 */
static AstStatement CheckLocalCompound (AstStatement stmt)
{
	AstStatement s;

	EnterScope ();
	s = CheckCompoundStatement (stmt);
	ExitScope();

	return stmt;
}

/* 并行语句的语义检查 */
static AstStatement CheckParallelStatement (AstStatement stmt)
{
    AstParallelStatement s = AsParall(stmt);

    s->stmt = CheckStatement (s->stmt);

    return stmt;
}

/* 各种状态的函数数组(对应ast.h) */
static AstStatement (*StmtCheckers[])(AstStatement) = 
{
	CheckExpressionStatement,
	CheckLabelStatement,
	CheckCaseStatement,
	CheckDefaultStatement,
	CheckIfStatement,
	CheckSwitchStatement,
	CheckLoopStatement,
	CheckLoopStatement,
	CheckForStatement,
	CheckGotoStatement,
	CheckBreakStatement,
	CheckContinueStatement,
	CheckReturnStatement,
	CheckLocalCompound, 
    CheckParallelStatement
};

/* 各种状态的语义检查 */
static AstStatement CheckStatement (AstStatement stmt)
{
    /* 各自状态对应各自的处理函数 */
	return (*StmtCheckers[stmt->kind - NK_ExpressionStatement])(stmt);
}

/* 复合语句检查 */
AstStatement CheckCompoundStatement (AstStatement stmt)
{
    /* 强制类型转换赋值 */
	AstCompoundStatement compStmt = AsComp (stmt);
	AstNode p;

	compStmt->ilocals = CreateVector (1);
    /* 检查局部变量 */
	for (p = compStmt->decls; p; p = p->next)
		CheckLocalDeclaration ((AstDeclaration)p, compStmt->ilocals);
	
    /* 检查语句 */
	for (p = compStmt->stmts; p; p = p->next)
		CheckStatement ((AstStatement)p);

	return stmt;
}
