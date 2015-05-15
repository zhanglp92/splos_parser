/*************************************************************************
	> File Name: exprchk.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月21日 星期日 22时08分29秒

	> Description: 
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "expr.h"
#include "decl.h"


/* 交换表达式的两个操作数 */
#define SWAP_KIDS(expr)              \
{                                    \
    AstExpression t = expr->kids[0]; \
    expr->kids[0] = expr->kids[1];   \
    expr->kids[1] = t;               \
}

/* 将两个操作数转换成同类型 */
#define PERFORM_ARITH_CONVERSION(expr)                                  \
    expr->ty = CommonRealType (expr->kids[0]->ty, expr->kids[1]->ty);   \
    expr->kids[0] = Cast (expr->ty, expr->kids[0]);                     \
    expr->kids[1] = Cast (expr->ty, expr->kids[1]);

/* 报错处理 */
#define REPORT_OP_ERROR                                                 \
    Error(&expr->coord, "Invalid operands to %s", OPNames[expr->op]);   \
    expr->ty = T(INT);                                                  \
    return expr;                   


/* 表达式为空的常量 */
static int IsNullConstant(AstExpression expr)
{
	return expr->op == OP_CONST && expr->val.i[0] == 0;
}

/* 类型提升（小于int 则全部提升为int） */
AstExpression DoIntegerPromotion(AstExpression expr)
{
	return expr->ty->categ < INT ? Cast (T(INT), expr) : expr;
}

/* 构造偏移(增量) */
static AstExpression ScalePointerOffset (AstExpression offset, int scale)
{
	AstExpression   expr;
	union value     val;

    /* 构造指针偏移 */
	CREATE_AST_NODE(expr, Expression);
	expr->ty = offset->ty;
	expr->op = OP_MUL;
    /* 单位偏移量 */
	expr->kids[0] = offset;
    /* 偏移单位 */
	val.i[1] = 0;
	val.i[0] = scale;
	expr->kids[1] = Constant (offset->coord, offset->ty, val);

	return FoldConstant (expr);
}

/* 构造偏移（减小） */
static AstExpression PointerDifference (AstExpression diff, int size)
{
	AstExpression   expr;
	union value     val;

	CREATE_AST_NODE (expr, Expression);

	expr->ty = diff->ty;
	expr->op = OP_DIV;
	expr->kids[0] = diff;
	val.i[1] = 0;
	val.i[0] = size;
	expr->kids[1] = Constant (diff->coord, diff->ty, val);

	return expr;
}

/* id const str */
static AstExpression CheckPrimaryExpression (AstExpression expr)
{
	Symbol p;

    /* const 修饰符 */
	if (expr->op == OP_CONST)
		return expr;

    /* 字符串 */
	if (expr->op == OP_STR) {

		expr->op    = OP_ID;
		expr->val.p = AddString (expr->ty, expr->val.p);
		expr->lvalue = 1;
		return expr;
	}

    /* id */
    /* 在符号表中查找id */
	p = LookupID (expr->val.p);
	if (p == NULL) {

        /* id 不存在 */
		Error (&expr->coord, "Undeclared identifier: %s", expr->val.p);
		p = AddVariable (expr->val.p, T(INT), Level == 0 ? 0 : TK_AUTO);
		expr->ty = T(INT);
		expr->lvalue = 1;
	} else if (p->kind == SK_TypedefName) {

        /* typedef 的别名 */
		Error(&expr->coord, "Typedef name cannot be used as variable");
		expr->ty = T(INT);
	} else if (p->kind == SK_EnumConstant) {

        /* 枚举 */
		expr->op    = OP_CONST;
		expr->val   = p->val;
		expr->ty    = T(INT);
	} else {

		expr->ty    = p->ty;
		expr->val.p = p;
		expr->inreg = p->sclass == TK_REGISTER;
		expr->lvalue  = expr->ty->categ != FUNCTION;
	}

	return expr;
}

/* 滞前++, -- */
static AstExpression TransformIncrement (AstExpression expr)
{
	AstExpression   casgn;
	union value     val;
	
    /* 滞前++, -- */
	val.i[1] = 0; 
    val.i[0] = 1;
	CREATE_AST_NODE(casgn, Expression);
	casgn->coord = expr->coord;
	casgn->op = (expr->op == OP_POSTINC || expr->op == OP_PREINC) ? OP_ADD_ASSIGN : OP_SUB_ASSIGN;
	casgn->kids[0] = expr->kids[0];
	casgn->kids[1] = Constant (expr->coord, T(INT), val);

	expr->kids[0] = CheckExpression (casgn);
	expr->ty = expr->kids[0]->ty;
	return expr;
}

/* 参数类型提升 */
static AstExpression PromoteArgument (AstExpression arg)
{
    /* 类型提升（如果级别低于int 提升到int 低于double 提升到double） */
	Type ty = Promote (arg->ty);

	return Cast (ty, arg);
}

/**
 * 检查参数
 * @param fty function type
 * @param arg argument expression
 * @param argNo the argument's number in function call
 * @param argFull if the function's argument is full
 */
static AstExpression CheckArgument (FunctionType fty, AstExpression arg, int argNo, int *argFull)
{
	Parameter param;
	int parLen = LEN(fty->sig->params);

	arg = Adjust (CheckExpression (arg), 1);
    /* 没有参数 */
	if (fty->sig->hasProto && parLen == 0) {

		*argFull = 1;
		return arg;
	}

    /* 常规参数处理完，有可变参 */
	if (argNo == parLen && !fty->sig->hasEllipse)
		*argFull = 1;
	
	if (!fty->sig->hasProto) {

        /* 参数类型提升 */
		arg = PromoteArgument (arg);
		if (parLen != 0) {

            /* argNo 从１开始 */
			param = GET_ITEM (fty->sig->params, argNo - 1);
            /* 检查参数类型兼容(实参和形参的类型) */
			if (!IsCompatibleType (arg->ty, param->ty))
				goto err;
		}
		return arg;
	} else if (argNo <= parLen) {

		param = GET_ITEM (fty->sig->params, argNo - 1);
        /* 检查实参形参能否正确赋值 */
		if (!CanAssign (param->ty, arg))
			goto err;

		if (param->ty->categ < INT)
			arg = Cast(T(INT), arg);
		else
			arg = Cast(param->ty, arg);

		return arg;
	} else {

		return PromoteArgument (arg);
	}

err:
	Error(&arg->coord, "Incompatible argument");
	return arg;
}

/* 函数调用(检查实参形参的匹配) 
 * 外部调用 */
static AstExpression CheckFunctionCall (AstExpression expr)
{
	AstExpression arg;
	Type    ty;
	AstNode *tail;
	int     argNo, argFull;

    /* 判断系统调用 */
    expr->callNum = LookupSysCall (expr->kids[0]->val.p);

    /* 外部调用 */
	if (expr->kids[0]->op == OP_ID && LookupID (expr->kids[0]->val.p) == NULL) {

		expr->kids[0]->ty = DefaultFunctionType;
        /* 添加到函数列表中 */
		expr->kids[0]->val.p = AddFunction (expr->kids[0]->val.p, DefaultFunctionType, TK_EXTERN);
	} else {

        /* 检查语义 */
		expr->kids[0] = CheckExpression (expr->kids[0]);
	}

	expr->kids[0] = Adjust (expr->kids[0], 1);
	ty = expr->kids[0]->ty;

	if (!(IsPtrType (ty) && IsFunctionType (ty->bty))) {

		Error(&expr->coord, "The left operand must be function or function pointer");
	    ty = DefaultFunctionType;
	} else {

		ty = ty->bty;
	}

    /* 检查参数 */
	tail    = (AstNode *)&expr->kids[1];
	arg     = expr->kids[1];
	argNo   = 1;
	argFull = 0; /* 标志是否已经处理完 */
	for ( ; arg && !argFull; argNo++) {

		*tail = (AstNode)CheckArgument ((FunctionType)ty, arg, argNo, &argFull);
		tail = &(*tail)->next;
		arg = (AstExpression)arg->next;
	}
	*tail = NULL;

	if (arg != NULL) {

        /* 实参太多 */
		while (arg != NULL) {

			CheckExpression (arg);
			arg = (AstExpression)arg->next;
		}
		Error(&expr->coord, "Too many arguments");
	} else if (argNo < LEN(((FunctionType)ty)->sig->params)) {

        /* 实参太少 */
		Error(&expr->coord, "Too few arguments");
	}
	expr->ty = ty->bty;

	return expr;
}

/* . 和 -> 的操作 */
static AstExpression CheckMemberAccess (AstExpression expr)
{
	Type    ty;
	Field   fld;

	expr->kids[0] = CheckExpression (expr->kids[0]);
	if (expr->op == OP_MEMBER) {

        /* ．的处理 */
		expr->kids[0] = Adjust (expr->kids[0], 0);
		ty = expr->kids[0]->ty;
		if (! IsRecordType(ty)){

            /* 必须是复合类型 */
			REPORT_OP_ERROR;
		}
		expr->lvalue = expr->kids[0]->lvalue;
	} else {

        /* ->的处理 */
		expr->kids[0] = Adjust (expr->kids[0], 1);
		ty = expr->kids[0]->ty;
		if (!(IsPtrType (ty) && IsRecordType (ty->bty))) {

            /* 必须是指针且基类必须是复合类型 */
			REPORT_OP_ERROR;
		}
		ty = ty->bty;
		expr->lvalue = 1;
	}
	
    /* 查找是否有该字段 */
	fld = LookupField (Unqual (ty), expr->val.p);
	if (fld == NULL) {

		Error(&expr->coord, "struct or union member %s doesn't exsist", expr->val.p);
		expr->ty = T(INT);
		return expr;
	}
	expr->ty    = Qualify (ty->qual, fld->ty);
	expr->val.p = fld;
	expr->bitfld = fld->bits != 0;
	return expr;
}

/* 有后缀的操作 */
static AstExpression CheckPostfixExpression (AstExpression expr)
{
	switch (expr->op) {

        /* 下标操作 */
    	case OP_INDEX:
    		expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 1);
	    	expr->kids[1] = Adjust (CheckExpression (expr->kids[1]), 1);
    		if (IsIntegType(expr->kids[0]->ty)) {

                /* 第二个操作数为整形偏移量 */
	    		SWAP_KIDS(expr);
		    }

		    if (IsObjectPtr (expr->kids[0]->ty) && IsIntegType (expr->kids[1]->ty)) {

                /* 下标操作，第一个操作数必须是指针（数组名），第二个必须是整形 */
    			expr->ty = expr->kids[0]->ty->bty;
	    		expr->lvalue = 1;
		    	expr->kids[1] = DoIntegerPromotion (expr->kids[1]);
			    expr->kids[1] = ScalePointerOffset (expr->kids[1], expr->ty->size);
    			return expr;
	    	}
		    REPORT_OP_ERROR;

        /* 函数调用 */
	    case OP_CALL:
		    return CheckFunctionCall (expr);

        /* . 和 -> */
	    case OP_MEMBER: case OP_PTR_MEMBER:
	    	return CheckMemberAccess (expr);

        /* 滞后自增自减 */
	    case OP_POSTINC: case OP_POSTDEC:
		    return TransformIncrement (expr);

    	default:
	    	assert(0);
	}

	REPORT_OP_ERROR;
}


static AstExpression CheckTypeCast(AstExpression expr)
{
	Type ty;

	ty = CheckTypeName ((AstTypeName)expr->kids[0]);
	expr->kids[1] = Adjust (CheckExpression (expr->kids[1]), 1);

	if (!(BothScalarType (ty, expr->kids[1]->ty) || ty->categ == VOID)) {

		Error(&expr->coord, "Illegal type cast");
		return expr->kids[1];
	}
	return Cast(ty, expr->kids[1]);
}


/* 单目运算
 * (++, --, &, *, +, -, ~, !, sizeof, cast) */
static AstExpression CheckUnaryExpression (AstExpression expr)
{
	Type ty;

	switch (expr->op) {

        /* 滞前++, -- */
    	case OP_PREINC: case OP_PREDEC:
    		return TransformIncrement (expr);

        /* & */
    	case OP_ADDRESS:
    		expr->kids[0] = CheckExpression (expr->kids[0]);
	    	ty = expr->kids[0]->ty;

		    if (expr->kids[0]->op == OP_DEREF) {

                /* 操作数自带＊号(＆和＊相互抵消) */
    			expr->kids[0]->kids[0]->lvalue = 0;
	    		return expr->kids[0]->kids[0];
    		} else if (expr->kids[0]->op == OP_INDEX) {

                /* 操作数做下表操作 */
    			expr->kids[0]->op = OP_ADD;
	    		expr->kids[0]->ty = PointerTo(ty);
		    	expr->kids[0]->lvalue = 0;
			    return expr->kids[0];
    		} else if (IsFunctionType(ty) || 
			     (expr->kids[0]->lvalue && ! expr->kids[0]->bitfld && ! expr->kids[0]->inreg)) {

    			expr->ty = PointerTo(ty);
	    		return expr;
		    }
		    break;

        /* * */
	    case OP_DEREF:
		    expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 1);
    		ty = expr->kids[0]->ty;
	    	if (expr->kids[0]->op == OP_ADDRESS) {

                /* 操作数做取地址操作 */
    			expr->kids[0]->kids[0]->ty = ty->bty;
	    		return expr->kids[0]->kids[0];
		    } else if (expr->kids[0]->op == OP_ADD && expr->kids[0]->kids[0]->isarray) {

                /* 操作数做数组加操作 */
    			expr->kids[0]->op = OP_INDEX;
	    		expr->kids[0]->ty = ty->bty;
		    	expr->kids[0]->lvalue = 1;
			    return expr->kids[0];
    		}
	
            if (IsPtrType (ty)) {

    			expr->ty = ty->bty;
	    		if (IsFunctionType (expr->ty)) {

    				return expr->kids[0];
	    		}
		    	expr->lvalue = 1;
			    return expr;
    		}
	    	break;

        /* +, - */
	    case OP_POS: case OP_NEG:
    		expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 1);
	    	if (IsArithType (expr->kids[0]->ty)) {

    			expr->kids[0] = DoIntegerPromotion (expr->kids[0]);
	    		expr->ty = expr->kids[0]->ty;
		    	return expr->op == OP_POS ? expr->kids[0] : FoldConstant (expr);
		    }
		    break;

        /* ~ */
	    case OP_COMP:
    		expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
	    	if (IsIntegType(expr->kids[0]->ty)) {

    			expr->kids[0] = DoIntegerPromotion(expr->kids[0]);
	    		expr->ty = expr->kids[0]->ty;
		    	return FoldConstant(expr);
		    }
    		break;

        /* ! */
	    case OP_NOT:
		    expr->kids[0] = Adjust(CheckExpression(expr->kids[0]), 1);
    		if (IsScalarType(expr->kids[0]->ty)) {
                
    			expr->ty = T(INT);
	    		return FoldConstant(expr);
		    }
    		break;

        /* sizeof 操作 */
    	case OP_SIZEOF:
	    	if (expr->kids[0]->kind == NK_Expression) {

                /* sizeof 加表达式 */
    			expr->kids[0] = CheckExpression (expr->kids[0]);
                /* sizeof 不能有位段列表 */
			    if (expr->kids[0]->bitfld)
	    			goto err;
		    	ty = expr->kids[0]->ty;
    		} else {

                /* 检查类型名(sizeof 加类型) */
    			ty = CheckTypeName ((AstTypeName)expr->kids[0]);
	    	}
		    if (IsFunctionType(ty) || ty->size == 0)
			    goto err;

    		expr->ty = T(UINT);
	    	expr->op = OP_CONST;
		    expr->val.i[0] = ty->size;
    		return expr;

        /* cast 类型转换 */
    	case OP_CAST:
	    	return CheckTypeCast (expr);

    	default:
	    	assert(0);
	}

err:
	REPORT_OP_ERROR;

}

/* 错误的表达式 */
static AstExpression CheckErrorExpression (AstExpression expr)
{
	assert (0);
	return NULL;
}

/* 三目运算 */
static AstExpression CheckConditionalExpression (AstExpression expr)
{
	int     qual;
	Type    ty1, ty2;

    /* 调整检查操作数１ */
	expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 1);

    /* 基本类型(包括指针) */
	if (!IsScalarType(expr->kids[0]->ty)) {

		Error (&expr->coord, "The first expression shall be scalar type.");
	}

    /* 三目运算的后两个操作数 */
	expr->kids[1]->kids[0] = Adjust (CheckExpression(expr->kids[1]->kids[0]), 1);
	expr->kids[1]->kids[1] = Adjust (CheckExpression(expr->kids[1]->kids[1]), 1);

	ty1 = expr->kids[1]->kids[0]->ty;
	ty2 = expr->kids[1]->kids[1]->ty;
	if (BothArithType (ty1, ty2)) {

        /* 除指针的基本类型 */
		expr->ty = CommonRealType (ty1, ty2);
		expr->kids[1]->kids[0] = Cast (expr->ty, expr->kids[1]->kids[0]);
		expr->kids[1]->kids[1] = Cast (expr->ty, expr->kids[1]->kids[1]);

		return FoldConstant (expr);
	} else if (IsRecordType (ty1) && ty1 == ty2) {

        /* 结构体或公用体类型 */
		expr->ty = ty1;
	} else if (ty1->categ == VOID && ty2->categ == VOID) {

        /* void类型 */
		expr->ty = T(VOID);
	} else if (IsCompatiblePtr (ty1, ty2)) {

        /* 指针类型兼容 */
		qual = ty1->bty->qual | ty2->bty->qual;
		expr->ty  = PointerTo (Qualify(qual, CompositeType (Unqual(ty1->bty), Unqual(ty2->bty))));
	} else if (IsPtrType (ty1) && IsNullConstant (expr->kids[1]->kids[1])) {

        /* 指针和０ */
		expr->ty = ty1;
	} else if (IsPtrType (ty2) && IsNullConstant (expr->kids[1]->kids[0])) {

        /* 指针和０ */
		expr->ty = ty2;
	} else if (NotFunctionPtr (ty1) && IsVoidPtr (ty2) ||
	         NotFunctionPtr (ty2) && IsVoidPtr (ty1)) {

        /* 非函数指针和空指针 */
		qual = ty1->bty->qual | ty2->bty->qual;
		expr->ty = PointerTo(Qualify(qual, T(VOID)));
	} else {

		Error(&expr->coord, "invalid operand for ? operator.");
		expr->ty = T(INT);
	}

	return expr;
}

/* 运算乘 */
static AstExpression CheckMultiplicativeOP (AstExpression expr)
{
    /* 非指针的基本类型 */
	if (expr->op != OP_MOD && BothArithType (expr->kids[0]->ty, expr->kids[1]->ty))
		goto ok;

	if (expr->op == OP_MOD && BothIntegType (expr->kids[0]->ty, expr->kids[1]->ty))
		goto ok;

	REPORT_OP_ERROR;

ok:
	PERFORM_ARITH_CONVERSION (expr);
	return FoldConstant (expr);
}

/* 运算-操作 */
static AstExpression CheckSubOP (AstExpression expr)
{
	Type ty1, ty2;

	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;
	if (BothArithType (ty1, ty2)) {

		PERFORM_ARITH_CONVERSION (expr);
		return FoldConstant (expr);
	}

    /* 指针减操作 */
	if (IsObjectPtr (ty1) && IsIntegType (ty2)) {

		expr->kids[1] = DoIntegerPromotion (expr->kids[1]);
		expr->kids[1] = ScalePointerOffset (expr->kids[1], ty1->bty->size);
		expr->ty = ty1;
		return expr;
	}

    /* 兼容的指针类型 */
	if (IsCompatiblePtr (ty1, ty2)) {

		expr->ty = T(INT);
		expr = PointerDifference (expr, ty1->bty->size);
		return expr;
	}

	REPORT_OP_ERROR;
}

/* 运算＋的操作(普通加和指针加) */
static AstExpression CheckAddOP (AstExpression expr)
{
	Type ty1, ty2;

	if (expr->kids[0]->op == OP_CONST) {

        /* 第一个操作数是const类型，则交换 */
		SWAP_KIDS (expr);
	}
	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;

    /* 整形或浮点 */
	if (BothArithType (ty1, ty2)) {

		PERFORM_ARITH_CONVERSION (expr);
		return FoldConstant (expr);
	}

    /* 整形和指针 */
	if (IsObjectPtr(ty2) && IsIntegType(ty1)) {

        /* 交换位置 */
		SWAP_KIDS (expr);
		ty1 = expr->kids[0]->ty;
		goto left_ptr;
	}

    /* 指针和整形 */
	if (IsObjectPtr(ty1) && IsIntegType(ty2))
	{
left_ptr:
        /* 类型提升为整形 */
		expr->kids[1] = DoIntegerPromotion (expr->kids[1]);
        /* 构造偏移量 */
		expr->kids[1] = ScalePointerOffset (expr->kids[1], ty1->bty->size);
		expr->ty = ty1;
		return expr;
	}

	REPORT_OP_ERROR;
}


/* 位移操作<<, >> */
static AstExpression CheckShiftOP(AstExpression expr)
{
    /* 操作数必须都是整形 */
	if (BothIntegType (expr->kids[0]->ty, expr->kids[1]->ty)) {

        /* 操作数类型的提升（都保证在int级别以上） */
		expr->kids[0] = DoIntegerPromotion (expr->kids[0]);
		expr->kids[1] = DoIntegerPromotion (expr->kids[1]);
		expr->ty = expr->kids[0]->ty;
		return FoldConstant (expr);
	}

	REPORT_OP_ERROR;
}

/* 关系运算>, <, >=, <= */
static AstExpression CheckRelationalOP (AstExpression expr)
{
	Type ty1, ty2;
	
	expr->ty = T(INT);
	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;
    /* 非指针（整形浮点） */
	if (BothArithType (ty1, ty2)) {

		PERFORM_ARITH_CONVERSION (expr);
		expr->ty = T(INT);
		return FoldConstant (expr);
	}

    /* 指针（除函数指针），且指类类型兼容 */
	if (IsObjectPtr (ty1) && IsObjectPtr (ty2) && 
		IsCompatibleType (Unqual(ty1->bty), Unqual(ty2->bty))) {

		return expr;
	}

    /* 指针，且指类的大小为０，且指类类型兼容 */
	if (IsIncompletePtr (ty1) && IsIncompletePtr (ty2) &&
		IsCompatibleType(Unqual (ty1->bty), Unqual(ty2->bty))) {

		return expr;
	}

	REPORT_OP_ERROR;
}

/* 检查==, != 操作 */
static AstExpression CheckEqualityOP (AstExpression expr)
{
	Type ty1, ty2;

	expr->ty = T(INT);
	ty1 = expr->kids[0]->ty;
	ty2 = expr->kids[1]->ty;

    /* 非指针 */
    /* 均是整形或者浮点的处理 */
	if (BothArithType (ty1, ty2)) {

        /* 提升类型到相同 */
		PERFORM_ARITH_CONVERSION (expr);
		expr->ty = T(INT);
		return FoldConstant (expr);
	}

    /* 指针 */
    /* （是指针并且指类相同），（非函数指针和void类型的指针），
     * （指针和０）*/
	if (IsCompatiblePtr (ty1, ty2) ||
		NotFunctionPtr (ty1) && IsVoidPtr (ty2) ||
		NotFunctionPtr (ty2) && IsVoidPtr (ty1) ||
		IsPtrType (ty1) && IsNullConstant (expr->kids[1]) ||
		IsPtrType (ty2) && IsNullConstant (expr->kids[0])) {

		return expr;
	}

	REPORT_OP_ERROR;
}

/* 按位操作 */
static AstExpression CheckBitwiseOP (AstExpression expr)
{
    /* 按位操作两个操作数必须都是整形 */
	if (BothIntegType (expr->kids[0]->ty, expr->kids[1]->ty)) {

        /* 类型提升让两操作数类型相同 */
		PERFORM_ARITH_CONVERSION (expr);
		return FoldConstant (expr);
	}
	REPORT_OP_ERROR;
}

/* 二元逻辑运算 */
static AstExpression CheckLogicalOP(AstExpression expr)
{
    /* 两个操作数必须都是非void,非指针的基本类型 */
	if (BothScalarType (expr->kids[0]->ty, expr->kids[1]->ty)) {

		expr->ty = T(INT);
		return FoldConstant (expr);
	}

    /* 非以上类型报错 */
	REPORT_OP_ERROR;
}

/* 二元操作检查函数数组
 * (||, &&, |, ^, &, ==, !=, >, <, >=, <=, <<, >>, +, -, *, /, %) */
static AstExpression (*BinaryOPCheckers[]) (AstExpression) = 
{
	CheckLogicalOP,
	CheckLogicalOP,
	CheckBitwiseOP,
	CheckBitwiseOP,
	CheckBitwiseOP,
	CheckEqualityOP,
	CheckEqualityOP,
	CheckRelationalOP,
	CheckRelationalOP,
	CheckRelationalOP,
	CheckRelationalOP,
	CheckShiftOP,
	CheckShiftOP,
	CheckAddOP,
	CheckSubOP,
	CheckMultiplicativeOP,
	CheckMultiplicativeOP,
	CheckMultiplicativeOP
};


/* 二元算数表达式 */
static AstExpression CheckBinaryExpression (AstExpression expr)
{
    if ( !expr ) return NULL;

    /* 检查调整两个操作数 */
	expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 1);;
	expr->kids[1] = Adjust (CheckExpression (expr->kids[1]), 1);

    /* 检查二元操作 */
	return (*BinaryOPCheckers[expr->op - OP_OR]) (expr);
}

/* 表达式的类型强制转换 */
static AstExpression CastExpression (Type ty, AstExpression expr)
{
	AstExpression cast;

	if (expr->op == OP_CONST && ty->categ != VOID)
		return FoldCast (ty, expr);

	CREATE_AST_NODE (cast, Expression);

	cast->coord = expr->coord;
	cast->op    = OP_CAST;
    /* 转换成ty 的类型 */
	cast->ty    = ty;
	cast->kids[0] = expr;

	return cast;
}

/* 强制类型转换 */
AstExpression Cast (Type ty, AstExpression expr)
{
    /* 取类型，表达式的类型标号 */
	int scode = TypeCode (expr->ty);
	int dcode = TypeCode (ty);
	
    /* 变量的类型是void 转换表达式的类型 */
	if (dcode == V) 
		return CastExpression (ty, expr);

    /* 两个类型所占的字节数相同 */
	if (scode < F4 && dcode < F4 && scode / 2 == dcode / 2) {

		expr->ty = ty;
		return expr;
	}

    /* 表达式的类型那个占一个或者两个字节，强制类型转换成int */
	if (scode < I4) {

		expr = CastExpression (T(INT), expr);
		scode = I4;
	}

	if (scode != dcode) {

		if (dcode < I4) {
			expr = CastExpression (T(INT), expr);
		}
		expr = CastExpression (ty, expr);
	}
	return expr;
}



/* 能否正确分配值给各个字段 */
int CanAssign (Type lty, AstExpression expr)
{
	Type    rty = expr->ty;

    /* 左值的类型 */
	lty = Unqual (lty);
    /* 表达式的类型 */
	rty = Unqual (rty);

    /* 类型相同可直接赋值 */
	if (lty == rty) 
		return 1;

    /* 除指针,void 之外的其他基本类型(可直接赋值) */
	if (BothArithType (lty, rty))
		return 1;
	
    /* 兼容的指针类型,且以表达式的类型修饰符为基准判断 */
	if (IsCompatiblePtr (lty, rty) && ((lty->bty->qual & rty->bty->qual) == rty->bty->qual)) 
		return 1;

    /* 非函数指针,void类型指针,[以表达式的类型修饰符为基准] */
	if ((NotFunctionPtr (lty) && IsVoidPtr(rty) || NotFunctionPtr(rty) && IsVoidPtr(lty))&& 
		((lty->bty->qual & rty->bty->qual) == rty->bty->qual))
		return 1;
	
    /* 变量是指针,表达式为空的常量 */
	if (IsPtrType(lty) && IsNullConstant(expr))
	    return 1;

    /* 两都是指针类型 */
	if (IsPtrType(lty) && IsPtrType(rty)) {

		Warning(&expr->coord, "assignment from incompatible pointer type");
		return 1;
	}

    /* 指针,和整形 || 整形,和指针 && 大小相等 */
	if ((IsPtrType(lty) && IsIntegType(rty) || IsPtrType(rty) && IsIntegType(lty))&&
		(lty->size == rty->size)) {

		Warning(&expr->coord, "conversion between pointer and integer without a cast");
		return 1;
	}

	return 0;
}

/* 设置表达式的属性, 设置左值, 表达式的类型, 及判断函数类型,数组类型 */
/* expr   : 表达式
 * rvalue : 右值(左值0, 右值1)
 * */
AstExpression Adjust (AstExpression expr, int rvalue)
{
    if ( !expr ) return Constant0;

	if (rvalue) {

		expr->ty = Unqual (expr->ty);
        /* 左值 */
		expr->lvalue = 0;
	}

	if (expr->ty->categ == FUNCTION) {

		expr->ty = PointerTo (expr->ty);
		expr->isfunc = 1;
	} else if (expr->ty->categ == ARRAY) {

		expr->ty = PointerTo (expr->ty->bty);
		expr->lvalue    = 0;
		expr->isarray   = 1;
	}

	return expr;
}


/* 逗号表达式语义检查 */
static AstExpression CheckCommaExpression (AstExpression expr)
{
    /* 检查两个操作数的语义, 调整设置为右值 */
	expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 1);
	expr->kids[1] = Adjust (CheckExpression (expr->kids[1]), 1);

    /* 表达式的类型修改成逗号表达式的第二个操作数的类型 */
	expr->ty = expr->kids[1]->ty;
	return expr;
}

/* 检查表达式是否能被修改(左值)(返回０则表示不能修改) */
static int CanModify (AstExpression expr)
{		
    /* 左值, 没被const修饰, 复合类型中没有cosnt字段 
     * 3个条件逗满足可以被修改 */
	return expr && (expr->lvalue && !(expr->ty->qual & CONST) && 
	        (IsRecordType (expr->ty) ? !((RecordType)expr->ty)->hasConstFld : 1));
}

/* 赋值表达式语义检查 */
static AstExpression CheckAssignmentExpression (AstExpression expr)
{
	int ops[] = { 

        /* |, ^, &, <<, >>, +, -, *, /, %(算数表达式可以直接＝的操作) */
		OP_BITOR, OP_BITXOR, OP_BITAND, OP_LSHIFT, OP_RSHIFT, 
		OP_ADD,	  OP_SUB,    OP_MUL,    OP_DIV,    OP_MOD 
	};
	Type ty;
	
    /* 检查表达式, 设置左右值属性 */
	expr->kids[0] = Adjust (CheckExpression (expr->kids[0]), 0);
	expr->kids[1] = Adjust (CheckExpression (expr->kids[1]), 1);

    /* 检查左值能否被修改 */
	if (!CanModify (expr->kids[0])) {

		Error(&expr->coord, "The left operand cannot be modified");
	}

    /* 连等赋值操作, 修改表达式先计算在赋值(两步完成) */
	if (expr->op != OP_ASSIGN) {

		AstExpression lopr;

		CREATE_AST_NODE (lopr, Expression);
		lopr->coord   = expr->coord;
        /* 将连等赋值操作，改成先计算再赋值 */
		lopr->op      = ops[expr->op - OP_BITOR_ASSIGN];
		lopr->kids[0] = expr->kids[0];
		lopr->kids[1] = expr->kids[1];

        /* 重新赋值给第二个操作数 */
		expr->kids[1] = (*BinaryOPCheckers[lopr->op - OP_OR])(lopr);
	}

    /* 左值类型 */
	ty = expr->kids[0]->ty;
	if (!CanAssign (ty, expr->kids[1])) {

		Error(&expr->coord, "Wrong assignment");
	} else {

        /* 给表达式进行强制类型转换 */
		expr->kids[1] = Cast (ty, expr->kids[1]);
	}
    /* 左值的类型, 及表达式的类型 */
	expr->ty = ty;
	return expr;
}

/* 表达式处理函数数组 */
static AstExpression (*ExprCheckers[])(AstExpression) = 
{
    #define OPINFO(op, prec, name, func, opcode) Check ##func##Expression,
    #include "opinfo.h"
    #undef OPINFO
};

/* 检查表达式的类型 */
AstExpression CheckExpression (AstExpression expr)
{
    return expr ? (*ExprCheckers[expr->op]) (expr) : NULL;
}

AstExpression CheckConstantExpression (AstExpression expr)
{
	expr = CheckExpression (expr);
	if (!expr || !(expr->op == OP_CONST && IsIntegType (expr->ty))) {

		return NULL;
	}
	return expr;
}
