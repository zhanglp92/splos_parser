/*************************************************************************
	> File Name: fold.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月21日 星期日 20时37分01秒

	> Description: 常量折叠
 ************************************************************************/

#include "pcl.h"
#include "ast.h"
#include "expr.h"



#define EXECUTE_BOP(op)                                                                      \
    if (tcode == I4) val.i[0] = expr1->val.i[0] op expr2->val.i[0];                          \
    else if (tcode == U4) val.i[0] = (unsigned)expr1->val.i[0] op (unsigned)expr2->val.i[0]; \
    else if (tcode == F4) val.f = expr1->val.f op expr2->val.f;                              \
    else if (tcode == F8) val.d = expr1->val.d op expr2->val.d;

#define EXECUTE_ROP(op)                                                                      \
    if (tcode == I4) val.i[0] = expr1->val.i[0] op expr2->val.i[0];                          \
    else if (tcode == U4) val.i[0] = (unsigned)expr1->val.i[0] op (unsigned)expr2->val.i[0]; \
    else if (tcode == F4) val.i[0] = expr1->val.f op expr2->val.f;                           \
    else if (tcode == F8) val.i[0] = expr1->val.d op expr2->val.d;


/**
 * Cast constant expression. e.g. (float)3
 */
/* 强制类型转换(基本类型的转换) */
AstExpression FoldCast (Type ty, AstExpression expr)
{
	int scode = TypeCode (expr->ty);
	int dcode = TypeCode (ty);

	switch (scode) {

        /* 类型已被提升为int */
    	case I1: case U1: case I2: case U2:
	    	break;

        /* 由int 转换 */
    	case I4:
	    	if (dcode == F4) 
    			expr->val.f = (float)expr->val.i[0];
	    	else if (dcode == F8)
    			expr->val.d = (double)expr->val.i[0];
	    	else if (dcode == I1)
		    	expr->val.i[0] = (char)expr->val.i[0];
    		else if (dcode == U1)
	    		expr->val.i[0] = (unsigned char)expr->val.i[0];
		    else if (dcode == I2)
			    expr->val.i[0] = (short)expr->val.i[0];
    		else if (dcode == U2)
	    		expr->val.i[0] = (unsigned short)expr->val.i[0];
		    break;

        /* 由unsigned int 转换 */
    	case U4:
	    	if (dcode == F4)
			    expr->val.f = (float)(unsigned)expr->val.i[0];
    		else if (dcode == F8)
	    		expr->val.d = (double)(unsigned)expr->val.i[0];
		    break;

        /* 由float转换 */
    	case F4:
    		if (dcode == I4)
	    		expr->val.i[0] = (int)expr->val.f;
		    else if (dcode == U4)
			    expr->val.i[0] = (unsigned)expr->val.f;
    		else
	    		expr->val.d = (double)expr->val.f;
		    break;

        /* 由double 转换 */
	    case F8:
    		if (dcode == I4)
	    		expr->val.i[0] = (int)expr->val.d;
		    else if (dcode == U4)
			    expr->val.i[0] = (unsigned)expr->val.d;
    		else
	    		expr->val.f = (float)expr->val.d;
		    break;

    	default:
	    	assert(0);
	}

	expr->ty = ty;
	return expr;
}

/**
 * Constant folding. e.g. 3 + 4
 */
/* 浮点常量 */
AstExpression FoldConstant (AstExpression expr)
{
	int             tcode;
	union value     val;
	AstExpression   expr1, expr2;
	
    /* 双目运算，且两个操作数均为const */
	if (expr->op >= OP_OR && expr->op <= OP_MOD &&
	    ! (expr->kids[0]->op == OP_CONST && expr->kids[1]->op == OP_CONST))
		return expr;

    /* ＋－！~，且操作数非const */
	if (expr->op >= OP_POS && expr->op <= OP_NOT && expr->kids[0]->op != OP_CONST)
		return expr;

    /* ? 表达式（三目运算符） */
	if (expr->op == OP_QUESTION) {

        /* 第一个操作数，const 切为整形 */
		if (expr->kids[0]->op == OP_CONST && IsIntegType (expr->kids[0]->ty)) {

            /* 返回三目运算的结果 */
			return expr->kids[0]->val.i[0] ? expr->kids[1]->kids[0] :expr->kids[1]->kids[1];
		}
		return expr;
	}

    /* 其他的表达式 */
	val.i[1] = val.i[0] = 0;
	tcode = TypeCode (expr->kids[0]->ty);
	expr1 = expr->kids[0];
	expr2 = expr->kids[1];

	switch (expr->op) {

        /* 或操作 */
    	case OP_OR:
	    	EXECUTE_ROP(||);
		    break;

        /* 与操作 */
    	case OP_AND:
	    	EXECUTE_ROP(&&);
		    break;

        /* 按位或操作 */
    	case OP_BITOR:
	    	val.i[0] = expr1->val.i[0] | expr2->val.i[0];
		    break;

        /* 按位异或操作 */
    	case OP_BITXOR:
	    	val.i[0] = expr1->val.i[0] ^ expr2->val.i[0];
		    break;

        /* 按位与操作 */
    	case OP_BITAND:
	    	val.i[0] = expr1->val.i[0] & expr2->val.i[0];
		    break;

    	case OP_EQUAL:
	    	EXECUTE_ROP(==);
		    break;

    	case OP_UNEQUAL:
	    	EXECUTE_ROP(!=);
		    break;

    	case OP_GREAT:
	    	EXECUTE_ROP(>);
		    break;

    	case OP_LESS:
	    	EXECUTE_ROP(<);
		    break;

    	case OP_GREAT_EQ:
	    	EXECUTE_ROP(>=);
		    break;

    	case OP_LESS_EQ:
	    	EXECUTE_ROP(<=);
		    break;

    	case OP_LSHIFT:
	    	val.i[0] = expr1->val.i[0] << expr2->val.i[0];
		    break;

    	case OP_RSHIFT:
	    	if (tcode == U4)
		    	val.i[0] = (unsigned)expr1->val.i[0] >> expr2->val.i[0];
    		else
	    		val.i[0] = expr1->val.i[0] >> expr2->val.i[0];
		    break;

    	case OP_ADD:
	    	EXECUTE_BOP(+);
		    break;
		
    	case OP_SUB:
	    	EXECUTE_BOP(-);
		    break;

    	case OP_MUL:
	    	EXECUTE_BOP(*);
		    break;

    	case OP_DIV:
	    	EXECUTE_BOP(/);
		    break;

    	case OP_MOD:
	    	if (tcode == U4)
		    	val.i[0] = (unsigned)expr1->val.i[0] % expr2->val.i[0];
    		else
	    		val.i[0] = expr1->val.i[0] % expr2->val.i[0];
		    break;

        /* 负号 */
    	case OP_NEG:
	    	if (tcode == I4 || tcode == U4)
		    	val.i[0] = -expr1->val.i[0];
    		else if (tcode == F4)
	    		val.f = -expr1->val.f;
		    else if (tcode == F8)
			    val.d = -expr->val.d;
    		break;

        /* 取反 */
	    case OP_COMP:
		    val.i[0] = ~expr1->val.i[0];
    		break;

        /* 非 */
    	case OP_NOT:
	    	if (tcode == I4 || tcode == U4)
		    	val.i[0] = !expr1->val.i[0];
    		else if (tcode == F4)
	    		val.i[0] = !expr1->val.f;
		    else if (tcode == F8)
			    val.i[0] = !expr1->val.d;
    		break;

    	default:
	    	assert(0);
		    return expr;
	}
	return Constant (expr->coord, expr->ty, val);
}


/* 构造一个常量值 */
AstExpression Constant (struct coord coord, Type ty, union value val)
{
    AstExpression expr;

    CREATE_AST_NODE (expr, Expression);
    expr->coord = coord;
    expr->ty = ty;
    expr->op = OP_CONST;
    expr->val = val;

    return expr;
}
