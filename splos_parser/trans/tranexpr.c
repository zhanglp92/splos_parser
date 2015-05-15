/*************************************************************************
	> File Name: tranexpr.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年01月28日 星期三 14时09分09秒

	> Description: 
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "expr.h"
#include "gen.h"

/* 翻译一个基本表达式, id, const, str
 * 返回其值的符号 */
static Symbol TranslatePrimaryExpression (AstExpression expr)
{
    /* 如果是常量,则将其添加到常量符号表中 
     * 并将其构造的符号作为返回值 */
	if (expr->op == OP_CONST)
		return AddConstant (expr->ty, expr->val);

    /* 如果是字符串,数组,函数,则构造地址符号将其返回 */
	if (expr->op == OP_STR || expr->isarray || expr->isfunc)
		return AddressOf (expr->val.p);

	return expr->val.p;
}

/* 读取一个位段的值.
 * fld 位段列表, p 未计算的值 */
static Symbol ReadBitField (Field fld, Symbol p)
{
	int size  = 8 * fld->ty->size;
    /* 描述位段在整个变量中的位置 */
	int lbits = size - (fld->bits + fld->pos);
	int rbits = size - (fld->bits);

	/**
	 * 如下例子,代码和图示:
	 * struct st {
     *
	 *   int a;
	 *   int b : 8;
	 * } st;    
	 * |        |bits(8)|    pos(32)   |
	 * ---------------------------------
	 * |        |  b    |              |
	 * ---------------------------------
	 */
    /* 先左移再右移计算位段的值,(并优化) */
	p = Simplify (T(INT), LSH, p, IntConstant (lbits));
	p = Simplify (T(INT), RSH, p, IntConstant (rbits));

	return p;
}

/* 填写字段 */
static Symbol WriteBitField (Field fld, Symbol dst, Symbol src)
{
    /* 对应字段列表的掩码 */
	int fmask = (1 << fld->bits) - 1;
	int mask  = fmask << fld->pos;
	Symbol p;

	if (src->kind == SK_Constant && src->val.i[0] == 0) {

        /* 字段值为0 */
		p = Simplify (T(INT), BAND, dst, IntConstant (~mask));
	} else if (src->kind == SK_Constant && (src->val.i[0] & fmask) == fmask) {

        /* 字段值为全1 */
		p  = Simplify(T(INT), BOR, dst, IntConstant (mask));
	} else {

		if (src->kind == SK_Constant) {

            /* 如果是常量, 则直接计算 */
			src = IntConstant ((src->val.i[0] << fld->pos) & mask);
		} else {

            /* 先左移,在用掩码屏蔽其余位 */
			src = Simplify (T(INT), LSH, src, IntConstant (fld->pos));
			src = Simplify (T(INT), BAND, src, IntConstant (mask));
		}

        /* 屏蔽目标表达式的其余位,在将值填入位段中 */
		p = Simplify (T(INT), BAND, dst, IntConstant (~mask));
		p = Simplify (T(INT), BOR,  p, src);
	}

	if (dst->kind == SK_Temp && AsVar(dst)->def->op == DEREF) {

        /* 间接赋值 */
		Symbol addr = AsVar(dst)->def->src1;

		GenerateIndirectMove (fld->ty, addr, p);
		dst = Deref (fld->ty, addr);
	} else {

        /* 直接赋值 */
		GenerateMove (fld->ty, dst, p);
	}

    /* 返回字段的值 */
	return ReadBitField (fld, dst);
}

/* 偏移 */
static Symbol Offset (Type ty, Symbol addr, Symbol voff, int coff)
{
	if (voff != NULL) {

		voff = Simplify (T(POINTER), ADD, voff, IntConstant (coff));
		addr = Simplify (T(POINTER), ADD, addr, voff);
		return Deref (ty, addr);
	}

	if (addr->kind == SK_Temp && AsVar(addr)->def->op == ADDR) {

		return CreateOffset (ty, AsVar(addr)->def->src1, coff);
	}

	return Deref (ty, Simplify(T(POINTER), ADD, addr, IntConstant (coff)));
}

/**
 * 翻译条件语句的值
 * e.g. int a, b; a = a > b;
 * 构造新的基本块,trueBB,falseBB 
 *     if a > b goto trueBB
 * falseBB:
 *     t = 0;
 *     goto nextBB;
 * trueBB:
 *     t = 1;
 * nextBB:
 *     ...
 */
static Symbol TranslateBranchExpression (AstExpression expr)
{
	BBlock nextBB, trueBB, falseBB;
	Symbol t;

    /* 创建分支的基本块 */
	t = CreateTemp (expr->ty);
	nextBB  = CreateBBlock ();
	trueBB  = CreateBBlock ();
	falseBB = CreateBBlock ();

    /* 分支基本块的处理 */
	TranslateBranch (expr, trueBB, falseBB);

    /* 条件不成立的处理 */
	StartBBlock (falseBB);
    /* 条件不成立赋0 值 */
	GenerateMove (expr->ty, t, IntConstant(0));
    /* 条件不成立则跳转执行nextBB */
	GenerateJump (nextBB);

    /* 条件成立的处理 */
	StartBBlock(trueBB);
    /* 条件成立赋1 值 */
	GenerateMove(expr->ty, t, IntConstant(1));
    /* 成立则继续向下执行 */
	StartBBlock(nextBB);

	return t;
}

/* 下标表达式 */
static Symbol TranslateArrayIndex (AstExpression expr)
{
	AstExpression   p;
	Symbol  addr, dst, voff = NULL;
	int     coff = 0;

    /* 解析n 数组 */
	p = expr;
	do {

		if (p->kids[1]->op == OP_CONST) {

			coff += p->kids[1]->val.i[0];
		} else if (voff == NULL) {

			voff = TranslateExpression (p->kids[1]);
		} else {

			voff = Simplify (voff->ty, ADD, voff, TranslateExpression (p->kids[1]));
		}
		p = p->kids[0];
	} while (p->op == OP_INDEX && p->kids[0]->isarray);

	addr = TranslateExpression (p);
	dst  = Offset (expr->ty, addr, voff, coff);

	return expr->isarray ? AddressOf (dst) : dst;
}

/* 翻译函数调用 */
static Symbol TranslateFunctionCall (AstExpression expr)
{
	AstExpression arg;
	Symbol  faddr, recv;
	ILArg   ilarg;
    /* 函数参数 */
	Vector  args = CreateVector (4);

	expr->kids[0]->isfunc = 0;
	faddr = TranslateExpression (expr->kids[0]);
    /* 参数 */
	for (arg = expr->kids[1]; arg; arg = (AstExpression)arg->next) {

		CALLOC(ilarg);
		ilarg->sym = TranslateExpression (arg);
		ilarg->ty  = arg->ty;
		INSERT_ITEM (args, ilarg);
	}

    /* 函数返回值赋值,创建临时变量 */
	recv = NULL;
	if (expr->ty->categ != VOID) {

		recv = CreateTemp (expr->ty);
	}

	GenerateFunctionCall (expr->ty, recv, faddr, args, expr->callNum);

	return recv;
}

/* 构造类型 */
static Symbol TranslateMemberAccess (AstExpression expr)
{
	AstExpression p;
	Field   fld;
	Symbol  addr, dst;
	int     coff = 0;

	p = expr;
	if (p->op == OP_PTR_MEMBER) {

		fld  =  p->val.p;
		coff = fld->offset;
		addr = TranslateExpression (expr->kids[0]);
	} else {

		while (p->op == OP_MEMBER) {

			fld  = p->val.p;
			coff += fld->offset;
			p    = p->kids[0];
		}
		addr = AddressOf (TranslateExpression (p));
	}

	dst = Offset(expr->ty, addr, NULL, coff);
	fld = dst->val.p = expr->val.p;

	if (fld->bits != 0 && expr->lvalue == 0)
		return ReadBitField(fld, dst);

	return expr->isarray ? AddressOf(dst) : dst;
}

/* 自增减 */
static Symbol TranslateIncrement (AstExpression expr)
{
	AstExpression casgn;
	Symbol  p;
	Field   fld = NULL;

	casgn = expr->kids[0];
	p = TranslateExpression (casgn->kids[0]);
	casgn->kids[0]->op      = OP_ID;
	casgn->kids[0]->val.p   = p;
	fld = p->val.p;

    /* 滞后自增减 */
	if (expr->op == OP_POSTINC || expr->op == OP_POSTDEC) {

		Symbol ret;
		ret = p;

		if (casgn->kids[0]->bitfld) {

			ret = ReadBitField (fld, p);
		} else if (p->kind != SK_Temp) {

			ret = CreateTemp (expr->ty);
			GenerateMove (expr->ty, ret, p);
		}
        /* 给casgn 进行+1 操作 */
		TranslateExpression (casgn);
        /* 返回原值 */
		return ret;
	}

    /* 直接+1 操作返回 */
	return TranslateExpression (casgn);
}

/* 翻译后缀表达式 */
static Symbol TranslatePostfixExpression (AstExpression expr)
{
	switch (expr->op) {

    	case OP_INDEX:
	    	return TranslateArrayIndex (expr);

    	case OP_CALL:
    		return TranslateFunctionCall (expr);

    	case OP_MEMBER: case OP_PTR_MEMBER:
    		return TranslateMemberAccess (expr);

	    case OP_POSTINC:    case OP_POSTDEC:
    		return TranslateIncrement (expr);

    	default:
	    	assert(0);
		    return NULL;
	}
}

/* 强制类型转换 
 * 将src 变量由类型sty 转换到ty 类型 */
static Symbol TranslateCast (Type ty, Type sty, Symbol src)
{
	Symbol  dst;
	int     scode, dcode, opcode;

	dcode = TypeCode (ty);
	scode = TypeCode (sty);

    /* 目标为void 类型不用转换 */
	if (dcode == V)
		return NULL;

	switch (scode) {

	    case I1: opcode = EXTI1; break;
    	case I2: opcode = EXTI2; break;
        case U1: opcode = EXTU1; break;
    	case U2: opcode = EXTU2; break;

        case I4: {
		    
            if (dcode <= U1)
			    opcode = TRUI1;
		    else if (dcode <= U2)
			    opcode = TRUI2;
    		else if (dcode == F4)
	    		opcode = CVTI4F4;
		    else if (dcode == F8)
			    opcode = CVTI4F8;
    		else
	    		return src;
        }break;

    	case U4: {
		
            if (dcode == F4)
    			opcode = CVTU4F4;
	    	else if (dcode == F8)
		    	opcode = CVTU4F8;
    		else
	    		return src;
        }break;

    	case F4: {

    		if (dcode == I4)
	    		opcode = CVTF4I4;
		    else if (dcode == U4)
			    opcode = CVTF4U4;
    		else
	    		opcode = CVTF4;
        }break;

	    case F8: {

    		if (dcode == I4)
	    		opcode = CVTF8I4;
		    else if (dcode == U4)
			    opcode = CVTF8U4;
    		else
	    		opcode = CVTF8;
        }break;

    	default:
	    	assert(0);
		    return NULL;
	}

	dst = CreateTemp (ty);
    /* 更改类型,赋值 */
	GenerateAssign (sty, dst, opcode, src, NULL);
	return dst;
}

/* 单目运算 */
static Symbol TranslateUnaryExpression (AstExpression expr)
{
	Symbol src;

    /* ! 运算,翻译条件语句,并返回条件的结果符号 */
	if (expr->op == OP_NOT) 
		return TranslateBranchExpression (expr);

    /* 滞前自增减 */
	if (expr->op == OP_PREINC || expr->op == OP_PREDEC)
		return TranslateIncrement (expr);

    /* 翻译操作数 */
	src = TranslateExpression (expr->kids[0]);
	switch (expr->op) {

    	case OP_CAST:
    		return TranslateCast (expr->ty, expr->kids[0]->ty, src);

    	case OP_ADDRESS:
	    	return AddressOf (src);

	    case OP_DEREF:
		    return Deref (expr->ty, src);

        /* 正/负号 */
    	case OP_NEG: case OP_COMP:
		    return Simplify (expr->ty, OPMap[expr->op], src, NULL);

    	default:
	    	assert(0);
		    return NULL;
	}
}

/* 二元运算 */
static Symbol TranslateBinaryExpression (AstExpression expr)
{
	Symbol src1, src2;

    /* ||, &&, 逻辑运算 */
	if (expr->op == OP_OR || expr->op == OP_AND || (expr->op >= OP_EQUAL && expr->op <= OP_LESS_EQ)) {

		return TranslateBranchExpression (expr);
	}

    /* 其余的二元运算,处理 */
	src1 = TranslateExpression (expr->kids[0]);
	src2 = TranslateExpression (expr->kids[1]);
	return Simplify (expr->ty, OPMap[expr->op], src1, src2);
}

/* 翻译条件表达式(处理分支) */
static Symbol TranslateConditionalExpression (AstExpression expr)
{
	Symbol t, t1, t2;
	BBlock trueBB, falseBB, nextBB;

	t = NULL;
	if (expr->ty->categ != VOID) {

		t = CreateTemp (expr->ty);
	}

    /* 条件(不)成立的基本块 */
	falseBB = CreateBBlock ();
	trueBB  = CreateBBlock ();
    /* 分支只有的基本块 */
	nextBB  = CreateBBlock ();

    /* 处理分支 */
	TranslateBranch (expr->kids[0], trueBB, falseBB);

    /* 处理条件不成立的基本块(将falseBB 块设置为当前块) */
	StartBBlock (falseBB);
	t1 = TranslateExpression (expr->kids[1]->kids[1]);
	if (t1 != NULL)
		GenerateMove (expr->ty, t, t1);
	GenerateJump (nextBB);

    /* 处理条件成立的基本块 */
	StartBBlock (trueBB);
	t2 = TranslateExpression (expr->kids[1]->kids[0]);
	if (t2 != NULL)
		GenerateMove (expr->ty, t, t2);
	StartBBlock (nextBB);

	return t;
}

/* 翻译赋值表达式 
 * lexpr = expr
 * 翻译左值, */
static Symbol TranslateAssignmentExpression (AstExpression expr)
{
	Symbol  dst, src;
	Field   fld = NULL;

    /* 返回操作数1 值符号表示 */
	dst = TranslateExpression (expr->kids[0]);
	fld = dst->val.p;

    /* 连等赋值 */
	if (expr->op != OP_ASSIGN) {

		expr->kids[0]->op = OP_ID;
        /* 判断位段给表达式重新赋值 */
		expr->kids[0]->val.p = expr->kids[0]->bitfld ? ReadBitField (fld, dst) : dst;
	}

    /* 返回操作数2 值符号表示 */
	src = TranslateExpression (expr->kids[1]);

    /* 给表达式1 赋值 */
	if (expr->kids[0]->bitfld) {

        /* 如果表达式1 是位段则, 填写位段赋值 */
		return WriteBitField (fld, dst, src);
	} else if (dst->kind == SK_Temp && AsVar(dst)->def->op == DEREF) {

		Symbol addr = AsVar(dst)->def->src1;
        /* 间接赋值给addr */
		GenerateIndirectMove (expr->ty, addr, src);
        /* 再给addr 做取址运算 */
		dst = Deref (expr->ty, addr);
	} else {

        /* 直接赋值 */
		GenerateMove (expr->ty, dst, src);
	}

	return dst;
}

/* 翻译逗号表达式 
 * expr1, expr2 
 * 翻译expr1 和expr2 将expr2 作为结果返回 */
static Symbol TranslateCommaExpression (AstExpression expr)
{
	TranslateExpression (expr->kids[0]);
	return TranslateExpression (expr->kids[1]);
}

/* 错误处理 */
static Symbol TranslateErrorExpression (AstExpression expr)
{
	assert (0);
	return NULL;
}

static Symbol (*ExprTrans[])(AstExpression) = 
{
    #define OPINFO(op, prec, name, func, opcode) Translate ##func##Expression,
    #include "opinfo.h"
    #undef OPINFO
};

/* 非操作 */
AstExpression Not (AstExpression expr)
{
	static int rops[] = { OP_UNEQUAL, OP_EQUAL, OP_LESS_EQ, OP_GREAT_EQ, OP_LESS, OP_GREAT };
	AstExpression t;

	switch (expr->op) {

        /* !(a || b) = !a && !b */
    	case OP_OR: 
    		expr->op      = OP_AND;
	    	expr->kids[0] = Not (expr->kids[0]);
		    expr->kids[1] = Not (expr->kids[1]);
    		return expr;

        /* !(a && b) = !a || !b */
    	case OP_AND:
	    	expr->op = OP_OR;
		    expr->kids[0] = Not (expr->kids[0]);
    		expr->kids[1] = Not (expr->kids[1]);
	    	return expr;

        /* (= => !=), (<= => >) ... */
    	case OP_EQUAL:  case OP_UNEQUAL:    case OP_GREAT:
    	case OP_LESS:   case OP_GREAT_EQ:   case OP_LESS_EQ:
    		expr->op = rops[expr->op - OP_EQUAL];
	    	return expr;

    	case OP_NOT:
	    	return expr->kids[0];

    	default:
	    	CREATE_AST_NODE (t, Expression);
		    t->coord = expr->coord;
    		t->ty    = T(INT);
	    	t->op    = OP_NOT;
		    t->kids[0] = expr;
    		return FoldConstant (t);
	}
}

/* 翻译分支语句 */
void TranslateBranch (AstExpression expr, BBlock trueBB, BBlock falseBB)
{
	BBlock rtestBB;
	Symbol src1, src2;
	Type ty;

	switch (expr->op) {

        /* 与操作分支 */
    	case OP_AND: {

    		rtestBB = CreateBBlock ();
            /* 短路运算 */
	    	TranslateBranch (Not (expr->kids[0]), falseBB, rtestBB);
		    StartBBlock (rtestBB);
            /* 与的第一个操作数为真,判断第二个操作数 */
    		TranslateBranch (expr->kids[1], trueBB, falseBB);
        }break;

    	case OP_OR: {
		
            rtestBB = CreateBBlock ();
		    TranslateBranch (expr->kids[0], trueBB, rtestBB);
    		StartBBlock (rtestBB);
	    	TranslateBranch (expr->kids[1], trueBB, falseBB);
        }break;

	    case OP_EQUAL:  case OP_UNEQUAL:    case OP_GREAT:
	    case OP_LESS:   case OP_GREAT_EQ:   case OP_LESS_EQ: {

    		src1 = TranslateExpression (expr->kids[0]);
	    	src2 = TranslateExpression (expr->kids[1]);
            /* 生成分支指令 */
		    GenerateBranch (expr->kids[0]->ty, trueBB, OPMap[expr->op], src1, src2);
        }break;

    	case OP_NOT: {

    		src1 = TranslateExpression (expr->kids[0]);
	    	ty = expr->kids[0]->ty;
		    if (ty->categ < INT) {

                /* 强制类型转换为int 类型 */
    			src1 = TranslateCast (T(INT), ty, src1);
	    		ty = T(INT);
		    }
		    GenerateBranch (ty, trueBB, JZ, src1, NULL);
        }break;

    	case OP_CONST: {

	    	if (expr->val.i[0] || expr->val.i[1]) {

                /* 非0 直接跳转 */
    			GenerateJump (trueBB);
	    	}
		}break;

	    default: {

    		src1 = TranslateExpression (expr);
	    	if (src1->kind  == SK_Constant) {

		    	if (src1->val.i[0] || src1->val.i[1]) {

                    GenerateJump (trueBB);
			    }
		    } else {

    			ty = expr->ty;
	    		if (ty->categ < INT) {

    				src1 = TranslateCast (T(INT), ty, src1);
	    			ty = T(INT);
		    	}
			    GenerateBranch (ty, trueBB, JNZ, src1, NULL);
		    }
        }break;
	}
}

/* 翻译表达式 */
Symbol TranslateExpression (AstExpression expr)
{
	return (*ExprTrans[expr->op])(expr);
}
