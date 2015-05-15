/*************************************************************************
	> File Name: transtmt.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月23日 星期二 22时40分45秒

	> Description: 语句翻译
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "decl.h"
#include "gen.h"
#include "stmt.h"
#include "expr.h"


static void TranslateStatement (AstStatement stmt);

#if 0
/**
 * This function decides if an expression has side effect.
 * Side effect means changes in the state of execution environment.
 * e.g. modifying an object or accessing a volatile object
 */
static int HasSideEffect(AstExpression expr)
{
	if (expr == NULL)
		return 0;

	// accessing a volatile object
	if (expr->op == OP_ID && expr->ty->qual & VOLATILE)
		return 1;

	// function call, a function may not have side effect but ucc doesn't check this
	if (expr->op == OP_CALL)
		return 1;

	// modifying an object
	if (expr->op >= OP_ASSIGN && expr->op <= OP_MOD_ASSIGN ||
	    expr->op == OP_PREINC  || expr->op == OP_PREDEC ||
	    expr->op == OP_POSTINC || expr->op == OP_POSTDEC)
		return 1;

	return HasSideEffect(expr->kids[0]) || HasSideEffect(expr->kids[1]);
}
#endif

/* 翻译表达式 */
static void TranslateExpressionStatement (AstStatement stmt)
{
	AstExpressionStatement exprStmt = AsExpr(stmt);

	if (exprStmt->expr != NULL) {

		TranslateExpression (exprStmt->expr);
	}
}

/* 翻译标签语句 */
static void TranslateLabelStatement (AstStatement stmt)
{
	AstLabelStatement labelStmt = AsLabel(stmt);

    /* 若标签没有被引用,则直接翻译后边的语句 */
	if (labelStmt->label->ref > 0) {

		if (labelStmt->label->respBB == NULL) {

			labelStmt->label->respBB = CreateBBlock ();
		}
        /* 将标签的基本块设置为当前基本块 */
		StartBBlock (labelStmt->label->respBB);
	}

    /* 翻译标签后的语句 */
	TranslateStatement (labelStmt->stmt);
}

/* 合并 switch case 语句 */
static int MergeSwitchBucket (SwitchBucket *pBucket)
{
	SwitchBucket bucket = *pBucket;
	int count = 0;

    /* 插入排序 */
	while ( bucket->prev ) {

		if ((bucket->prev->ncase + bucket->ncase + 1) * 2 <= (bucket->maxVal - bucket->prev->minVal))
			break;

		bucket->prev->ncase     += bucket->ncase;
		bucket->prev->maxVal    = bucket->maxVal;
		*bucket->prev->tail     = bucket->cases;
		bucket->prev->tail      = bucket->tail;
		bucket= bucket->prev;
		count++;
	}

	*pBucket = bucket;
	return count;
}


/**
 * Generates selection and jump code for an array of switch buckets using a binary search.
 * Given the following bucket array:
 * [0, 1, 2] [9, 11] [24]
 * First select [9, 11].
 * if choice < 9, goto left half [0, 1, 2]
 * if choice > 11, goto right half [24]
 * generate indirect jump to each case statement and default label
 */
 /* 折半查找 */
static void TranslateSwitchBuckets (SwitchBucket *bucketArray, int left, int right, Symbol choice, BBlock currBB, BBlock defBB)
{
	int     mid, len, i;
	AstCaseStatement p;
	BBlock  lhalfBB, rhalfBB;
	BBlock  *dstBBs;
	Symbol  index;

	if (left > right)
		return;

	mid = (left + right) / 2;
	lhalfBB = (left > mid - 1) ?  defBB : CreateBBlock();
	rhalfBB = (mid + 1 > right) ? defBB : CreateBBlock();

	len = bucketArray[mid]->maxVal - bucketArray[mid]->minVal + 1;

	dstBBs = HeapAllocate (CurrentHeap, (len + 1)* sizeof(BBlock));
	for (i = 0; i < len; ++i)
		dstBBs[i] = defBB;
	dstBBs[len] = NULL;

	for (p = bucketArray[mid]->cases; p; p = p->nextCase) { 

		i = p->expr->val.i[0] - bucketArray[mid]->minVal;
		dstBBs[i] = p->respBB;
	}

	if (currBB != NULL) {

		StartBBlock (currBB);
	}
	GenerateBranch (choice->ty, lhalfBB, JL, choice, IntConstant (bucketArray[mid]->minVal));
	StartBBlock (CreateBBlock());
	GenerateBranch (choice->ty, rhalfBB, JG, choice, IntConstant (bucketArray[mid]->maxVal));
	StartBBlock (CreateBBlock());

	if (len != 1) {

		index = CreateTemp (choice->ty);
		GenerateAssign (choice->ty, index, SUB, choice, IntConstant (bucketArray[mid]->minVal));
		GenerateIndirectJump (dstBBs, len, index);
	} else {

		GenerateJump (dstBBs[0]);
	}

	StartBBlock (CreateBBlock ());

    /* 二分递归 */
	TranslateSwitchBuckets (bucketArray, left, mid - 1, choice, lhalfBB, defBB);
	TranslateSwitchBuckets (bucketArray, mid + 1, right, choice, rhalfBB, defBB);
}

/**
 * Generate intermediate code for switch statement.
 * During semantic check, the case statements in a switch statement is already ordered in ascending order.
 * The first step of this function is to divide these case statements into switch buckets.
 * The dividing criteria is:
 * Each switch bucket holds some case statements, there must be a case statement with minimum value(minVal)
 * and a case statement with maximum value(maxVal). We define the density of the switch bucket to be:
 * density = number of case statements / (maxVal - minVal). The density of a switch bucket must be greater
 * than 1/2.
 * And when adding new case statements into a switch bucket, there is a chance that the switch bucket can be 
 * merged with previous switch buckets.
 * 
 * Given the following switch statement:
 * switch (a) { case 0: case 1: case 4: case 9: case 10: case 11: ... };
 * [0, 1, 4] will be the first bucket, since 3 / (4 - 0) > 1/2. 
 * 9 will starts a new bucket, since 4 / (9 - 0) < 1/2. But when encountering 11, 6 / (11 - 0) > 1/2
 * So the merged bucket will be [0, 1, 4, 9, 10, 11]
 *
 * The second step generates the selection and jump code to different switch buckets(See TranslateSwitchBuckets)
 * 
 * The final step generates the intermediate code for the enclosed statement.
 */
 /* 翻译switch 语句 
  * 创建case default 基本块 */
static void TranslateSwitchStatement (AstStatement stmt)
{
	AstSwitchStatement  swtchStmt = AsSwitch(stmt);
	AstCaseStatement    p, q;
	SwitchBucket        bucket, b;
	SwitchBucket        *bucketArray;
	int     i, val;
	Symbol  sym;

    /* 翻译switch 的表达式 */
    sym = TranslateExpression (swtchStmt->expr);

	bucket = b = NULL;
	p = swtchStmt->cases;

    /* 创建基本块, 并给case 语句排序 */
	while ( p ) {

		q = p;
		p = p->nextCase;

        /* 给case 创建基本块 */
		q->respBB = CreateBBlock ();
        /* case 的表达式 */
		val = q->expr->val.i[0];

		if (bucket && (bucket->ncase + 1) * 2 > (val - bucket->minVal)) {

			bucket->ncase++;
			bucket->maxVal  = val;
			*bucket->tail   = q;
			bucket->tail    = &(q->nextCase);
			swtchStmt->nbucket -= MergeSwitchBucket (&bucket);
		} else {

			CALLOC(b);

			b->cases    = q;
			b->ncase    = 1;
            /* 初始化最小大值 */
			b->minVal   = b->maxVal = val;
			b->tail     = &(q->nextCase);
			b->prev     = bucket;
			bucket      = b;
			swtchStmt->nbucket++;
		}
	}

	swtchStmt->buckets = bucket;

	bucketArray = HeapAllocate (CurrentHeap, swtchStmt->nbucket * sizeof (SwitchBucket));

	for (i = swtchStmt->nbucket - 1; i >= 0; i--) {

		bucketArray[i] = bucket;
		*bucket->tail  = NULL;
		bucket = bucket->prev;
	}

    /* default 语句 */
	swtchStmt->defBB = CreateBBlock();
	if (swtchStmt->defStmt) {

		swtchStmt->defStmt->respBB = swtchStmt->defBB;
		swtchStmt->nextBB = CreateBBlock();
	} else {

		swtchStmt->nextBB = swtchStmt->defBB;
	}

	TranslateSwitchBuckets (bucketArray, 0, swtchStmt->nbucket - 1, sym, NULL, swtchStmt->defBB);
	TranslateStatement (swtchStmt->stmt);
	StartBBlock (swtchStmt->nextBB);
}


/* 翻译case 语句 */
static void TranslateCaseStatement(AstStatement stmt)
{
	AstCaseStatement caseStmt = AsCase(stmt);

    /* 在switch 中已经创建 */
	StartBBlock (caseStmt->respBB);
	TranslateStatement (caseStmt->stmt);
}

/* 翻译default 语句 */
static void TranslateDefaultStatement (AstStatement stmt)
{
	AstDefaultStatement defStmt = AsDef(stmt);
	
    /* 在switch 中已经创建 */
	StartBBlock (defStmt->respBB);
	TranslateStatement (defStmt->stmt);
}


/* 翻译if 语句
 *
 * if (expr) stmt is translated into:
 *     if ! expr goto nextBB
 * trueBB:
 *     stmt
 * nextBB:
 *     ...     
 *
 * if (expr) stmt1 else stmt2 is translated into:
 *     if ! expr goto falseBB
 * trueBB:
 *     stmt1
 *     goto nextBB
 * falseBB:
 *     stmt2
 * nextBB:
 *     ...
 */
static void TranslateIfStatement(AstStatement stmt)
{
	AstIfStatement ifStmt = AsIf(stmt);
	BBlock nextBB;
	BBlock trueBB;
	BBlock falseBB;

	nextBB = CreateBBlock ();
	trueBB = CreateBBlock ();

	if (ifStmt->elseStmt == NULL) {

        /* 没有false 模块,如果条件不成立则直接跳到if 之后的模块 */
		TranslateBranch (Not (ifStmt->expr), nextBB, trueBB);
        /* 条件成立,进入if 内 */
		StartBBlock (trueBB);
        /* 翻译if 分支语句 */
		TranslateStatement (ifStmt->thenStmt);
	} else {

		falseBB = CreateBBlock ();

		TranslateBranch (Not(ifStmt->expr), falseBB, trueBB);
		StartBBlock (trueBB);
		TranslateStatement (ifStmt->thenStmt);
        /* if 语句出来直接跳转到 next */
		GenerateJump (nextBB);

        /* else 模块 */
		StartBBlock (falseBB);
		TranslateStatement (ifStmt->elseStmt);
	}

    /* if 之后的模块 */
	StartBBlock (nextBB);
}

/**
 * 翻译while 语句
 *
 * while (expr) stmt is translated into:
 * goto contBB
 * loopBB:
 *     stmt
 * contBB:
 *     if (expr) goto loopBB
 * nextBB:
 *     ...
 */
static void TranslateWhileStatement (AstStatement stmt)
{
	AstLoopStatement whileStmt = AsLoop(stmt);

    /* 循环体基本块 */
	whileStmt->loopBB = CreateBBlock ();
    /* 继续判断条件 */
	whileStmt->contBB = CreateBBlock ();
    /* 循环结束后基本块 */
	whileStmt->nextBB = CreateBBlock ();

    /* 条件 */
	GenerateJump (whileStmt->contBB);

    /* 循环体 */
	StartBBlock (whileStmt->loopBB);
	TranslateStatement (whileStmt->stmt);

    /* 判断条件,分支 */
	StartBBlock (whileStmt->contBB);
	TranslateBranch (whileStmt->expr, whileStmt->loopBB, whileStmt->nextBB);

	StartBBlock (whileStmt->nextBB);
}

/**
 * 翻译do 语句
 *
 * do stmt while (expr) is translated into:
 * loopBB:
 *     stmt
 * contBB:
 *     if (expr) goto loopBB
 * nextBB:
 *     ...
 */
static void TranslateDoStatement (AstStatement stmt)
{
	AstLoopStatement doStmt = AsLoop(stmt);

	doStmt->loopBB = CreateBBlock ();
	doStmt->contBB = CreateBBlock ();
	doStmt->nextBB = CreateBBlock ();

    /* do 语句先执行循环体 */
	StartBBlock (doStmt->loopBB);
	TranslateStatement (doStmt->stmt);

    /* 判断条件,分支 */
	StartBBlock (doStmt->contBB);
	TranslateBranch (doStmt->expr, doStmt->loopBB, doStmt->nextBB);

	StartBBlock (doStmt->nextBB);
}


/**
 * 翻译for 语句
 *
 * for (expr1; expr2; expr3) stmt is translated into
 *     expr1
 *     goto testBB
 * loopBB:
 *     stmt
 * contBB:
 *     expr3
 * testBB:
 *     if expr2 goto loopBB (goto loopBB if expr2 is NULL)
 * nextBB:
 *     ...
 * Please pay attention to the difference between for and while
 * The continue point and loop test point is same for a while statemnt,
 * but different for a for statment.
 */
static void TranslateForStatement(AstStatement stmt)
{
	AstForStatement forStmt = AsFor(stmt);

	forStmt->loopBB = CreateBBlock ();
	forStmt->contBB = CreateBBlock ();
	forStmt->testBB = CreateBBlock ();
	forStmt->nextBB = CreateBBlock ();

    /* for 的初始条件 */
	if (forStmt->initExpr) {

		TranslateExpression (forStmt->initExpr);
	}
    /* 跳转到条件判断 */
	GenerateJump (forStmt->testBB);

    /* 循环体 */
	StartBBlock (forStmt->loopBB);
	TranslateStatement (forStmt->stmt);

    /* 步长 */
	StartBBlock (forStmt->contBB);
	if (forStmt->incrExpr) {

		TranslateExpression (forStmt->incrExpr);
	}

    /* 测试条件,是否跳转 */
	StartBBlock (forStmt->testBB);
	if (forStmt->expr) {

		TranslateBranch (forStmt->expr, forStmt->loopBB, forStmt->nextBB);
	} else {

		GenerateJump (forStmt->loopBB);
	}

	StartBBlock (forStmt->nextBB);
}

/**
 * 翻译goto 语句
 *
 * goto label is translation into:
 *     goto labelBB
 * nextBB:
 *     ...
 */
static void TranslateGotoStatement (AstStatement stmt)
{
	AstGotoStatement gotoStmt = AsGoto(stmt);

    /* 要跳转到的标签基本块不存在,则创建 */
	if (gotoStmt->label->respBB == NULL) {

		gotoStmt->label->respBB = CreateBBlock ();
	}
    /* 跳转 */
	GenerateJump (gotoStmt->label->respBB);
	StartBBlock (CreateBBlock());
}

/**
 * 翻译break 语句
 * A break statement terminates the execution of associated
 * switch or loop.
 *
 * break is translated into:
 *     goto switch or loop's nextBB
 * nextBB:
 *     ...
 */
static void TranslateBreakStatement (AstStatement stmt)
{
	AstBreakStatement brkStmt = AsBreak(stmt);

	if (brkStmt->target->kind == NK_SwitchStatement) { 

        /* switch 在跳转 */
		GenerateJump (AsSwitch(brkStmt->target)->nextBB);
	} else {
    
        /* loop 跳转 */
		GenerateJump (AsLoop(brkStmt->target)->nextBB);
	}

	StartBBlock (CreateBBlock ());
}

/**
 * 翻译continue 语句
 * A continue statement causes next iteration of associated loop.
 *
 * continue is translate into:
 *    goto loop's contBB
 * nextBB:
 *    ...
 */
static void TranslateContinueStatement (AstStatement stmt)
{
	AstContinueStatement contStmt = AsCont(stmt);

	GenerateJump (contStmt->target->contBB);
	StartBBlock (CreateBBlock ());
}

/**
 * 翻译return 语句
 * A return statement terminates execution of current function.
 */
static void TranslateReturnStatement (AstStatement stmt)
{
	AstReturnStatement retStmt = AsRet(stmt);

	if (retStmt->expr) {

		GenerateReturn (retStmt->expr->ty, TranslateExpression (retStmt->expr));
	}

	GenerateJump (FSYM->exitBB);
	StartBBlock (CreateBBlock());
}

/* 翻译一个复合语句 */
static void TranslateCompoundStatement (AstStatement stmt)
{
	AstCompoundStatement compStmt = AsComp (stmt);
	AstNode p;
	Vector  ilocals = compStmt->ilocals;
	Symbol  v;
	int     i;

    /* 翻译局部变量 */
	for (i = 0; i < LEN (ilocals); ++i) {

		InitData    initd;
		Type        ty;
		Symbol      dst, src;
		int         size;

		v = GET_ITEM (ilocals, i);
		initd   = AsVar(v)->idata;
		size    = 0;
		while (initd != NULL) {

			if (initd->offset != size) {

				dst = CreateOffset (T(UCHAR), v, size);
				GenerateClear (dst, initd->offset - size);
			}

			ty = initd->expr->ty;
			if (initd->expr->op == OP_STR) {

				String str = initd->expr->val.p;
				src = AddString (ArrayOf (str->len + 1, ty->bty), str);
			} else {

				src = TranslateExpression (initd->expr);
			}
			dst = CreateOffset (ty, v, initd->offset);
            /* 赋值 */
			GenerateMove (ty, dst, src);

			size  = initd->offset + ty->size;
			initd = initd->next;
		}

		if (size < v->ty->size) {

			dst = CreateOffset (T(UCHAR), v, size);
			GenerateClear (dst, v->ty->size - size);
		}
	}

	for (p = compStmt->stmts; p; p = p->next) {

		TranslateStatement ((AstStatement)p);
	}
}

/* 翻译并行语句 */
static void TranslateParallelStatement (AstStatement stmt)
{
    AstParallelStatement    parall = AsParall(stmt);

    /* 设置并行块标志 */
    isParall = 1;
    parall->respBB = CreateBBlock ();

    StartBBlock (parall->respBB);
    /* 该模块中创建的所有块都是并行块 */
    TranslateStatement (parall->stmt);

    /* 并行块翻译完成 */
    isParall = 0;
    StartBBlock (CreateBBlock ());
}

/* 翻译每个基本状态函数的函数数组 */
static void (*StmtTrans[])(AstStatement) = 
{
	TranslateExpressionStatement,
	TranslateLabelStatement,
	TranslateCaseStatement,
	TranslateDefaultStatement,
	TranslateIfStatement,
	TranslateSwitchStatement,
	TranslateWhileStatement,
	TranslateDoStatement,
	TranslateForStatement,
	TranslateGotoStatement,
	TranslateBreakStatement,
	TranslateContinueStatement,
	TranslateReturnStatement,
	TranslateCompoundStatement,
    TranslateParallelStatement
};


/* 翻译每条语句 */
static void TranslateStatement (AstStatement stmt)
{
	(*StmtTrans[stmt->kind - NK_ExpressionStatement])(stmt);
}

/* 翻译一个函数 */
static void TranslateFunction (AstFunction func)
{
	BBlock bb;

	FSYM = func->fsym;
	if (!FSYM->defined)
		return;

	TempNum = 0;
    /* 创建函数中间代码的开始结束 */
	FSYM->entryBB   = CreateBBlock ();
	FSYM->exitBB    = CreateBBlock ();

    /* 当前中间代码的入口点 */
	CurrentBB = FSYM->entryBB;
    /* 翻译函数体中的语句 */
	TranslateStatement (func->stmt);

    /* 函数结束 */
	StartBBlock (FSYM->exitBB);

    /* 中间代码优化 */
	Optimize (FSYM);

    static int i = 0;

    /* 给基本块创建名字 */
    for (bb = FSYM->entryBB; bb; bb = bb->next) {

		if (bb->ref != 0) {

			bb->sym = CreateLabel ();
		}
	}
}

/* 语句翻译 */
void Translate (AstTranslationUnit transUnit)
{
	AstNode p = transUnit->extDecls;

	for ( ; p; p = p->next) {

        /* 翻译函数, 函数要有函数体 */
		if (p->kind == NK_Function && ((AstFunction)p)->stmt) {

			TranslateFunction ((AstFunction)p);
		}
	}
}
