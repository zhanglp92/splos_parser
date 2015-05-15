/*************************************************************************
	> File Name: simp.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年01月28日 星期三 14时24分54秒

	> Description: 优化
 ************************************************************************/

#include "pcl.h"
#include "gen.h"

/* u > 1 并且 u 是2的n次幂 
 * 返回n, 否则返回0 */
static int Power2 (unsigned int u)
{
	int n;

    /* u > 1 并且 u 是2的n次幂 */
	if (u > 1 && (u &(u - 1)) == 0) {

		for (n = 0; u; u >>= 1, n++) {

			if (u & 1)
				return n;
		}
	}
	return 0;
}

/* 算数优化 */
Symbol Simplify (Type ty, int opcode, Symbol src1, Symbol src2)
{
	VariableSymbol  t;
	Symbol          p1, p2;
	int             c1, c2;

    /* 浮点 */
	if (IsRealType (ty))
		goto add_value;

	if (src2 == NULL || (src2->kind != SK_Constant && opcode != SUB))
		goto add_value;

    /* 以下是 加, 减, 乘, 除, 取余, 左移, 右移, 按位操作的优化 */
	switch (opcode) {

    	case ADD: {
		
            /* a + 0 = a */
		    if (src2->val.i[0] == 0)
			    return src1;

		    /* a + c1 + c2 = a + (c1 + c2) */
    		/* a - c1 + c2 = a + (-c1 + c2) */
		    p1 = src1; c1 = 0;
    		if (src1->kind == SK_Temp) {

                t = AsVar(src1);
    			if (t->def->src2 && t->def->src2->kind == SK_Constant && 
    			    (t->def->op == ADD || t->def->op == SUB)) {

    				p1 = t->def->src1;
	    			c1 = (t->def->op == ADD ? 1 : -1) * t->def->src2->val.i[0];
			    }
		    }
		
            if (c1 != 0) {

    			src1 = p1;
                /* 合并c1, c2 */
	    		src2 = IntConstant (c1 + src2->val.i[0]);
		    }
        }break;

    	case SUB: {

    		/* a - 0 = a */
	    	if (src2->kind == SK_Constant && src2->val.i[0] == 0)
		    	return src1;

    		// put source operand into v + c format (v maybe NULL, c maybe 0)
    		p1 = src1; c1 = 0;
	    	if (src1->kind == SK_Temp) {

    			t = AsVar(src1);
	    		if (t->def->src2 && t->def->src2->kind == SK_Constant && 
		    	    (t->def->op == ADD || t->def->op == SUB)) {

    				p1 = t->def->src1;
	    			c1 = (t->def->op == ADD ? 1 : -1) * t->def->src2->val.i[0];
		    	}
		    } else if (src1->kind == SK_Constant) {

    			p1 = NULL;
	    		c1 = src1->val.i[0];
		    }

    		p2 = src2; c2 = 0;
	    	if (src2->kind == SK_Temp) {

    			t = AsVar(src2);
	    		if (t->def->src2 && t->def->src2->kind == SK_Constant && 
		    	    (t->def->op == ADD || t->def->op == SUB)) {

    				p2 = t->def->src1;
	    			c2 = (t->def->op == ADD ? 1 : -1) * t->def->src2->val.i[0];
		    	}
		    } else if (src2->kind == SK_Constant) {

    			p2 = NULL;
	    		c2 = src2->val.i[0];
		    }

		    if (p1 == p2) {

    			/* (a + c1) - (a + c2) = c1 - c2 */
	    		return IntConstant (c1 - c2);
		    } else if (p1 == NULL) {

                /* c1 - (a + c2) = (c1 - c2) - a */
			    src1 = IntConstant (c1 - c2);
			    src2 = p2;
		    } else if (p2 == NULL) {

    			/* (a + c1) - c2 = a + (c1 - c2) */
	    		src1    = p1;
		    	opcode  = ADD;
			    src2    = IntConstant(c1 - c2);
		    }
        }break;

    	case MUL: case DIV: {

    		/* a * 1 = a; a / 1 = a; */
	    	if (src2->val.i[0] == 1)
		    	return src1;

    		/* a * 2 power of n = a >> n */
	    	c1 = Power2 (src2->val.i[0]);
		    if (c1 != 0) {
			
                /* 转换乘除为位移操作 */
                src2   = IntConstant (c1);
			    opcode = opcode == MUL ? LSH : RSH;
		    }
        }break;

	    case MOD: {

		    /* a % 1 = 0 */
		    if (src2->val.i[0] == 1)
			    return IntConstant (0);

    		/* a % 2 power of n = a & (2 power of n - 1) */
	    	c1 = Power2 (src2->val.i[0]);
		    if (c1 != 0) {

    			src2 = IntConstant (src2->val.i[0] - 1);
	    		opcode = BAND;
		    }
        }break;

    	case LSH: case RSH: {

    		/* a >> 0 = a << 0 = a */
	    	if (src2->val.i[0] == 0)
		    	return src1;
    	}break;

    	case BOR: {

            /* a | 0 = a; a | -1 = -1 */
    		if (src2->val.i[0] == 0)
	    		return src1;
		    if (src2->val.i[0] == -1)
			    return src2;
        }break;

    	case BXOR: {
	
            /* a ^ 0 = a */
		    if (src2->val.i[0] == 0)
    			return src1;
        }break;

    	case BAND: {

            /* a & 0 = 0, a & -1 = a */
    		if (src2->val.i[0] == 0)
	    		return IntConstant(0);
            if (src2->val.i[0] == -1)
			    return src1;
        }break;

    	default: break;
	}

add_value:
    /* 构造值添加到符号表中,并返回 */
	return TryAddValue (ty, opcode, src1, src2);
}

/* 窥孔优化
 * 这部分优化包括识别 a = a + 1 这样的指令将其变成对 a 的自增操作。
 * 还有比如对于 a = f(1)产生的中间代码为:
 * t = f(1); a = t;
 * 将其合并为 a = f(1) */
static void PeepHole (BBlock bb)
{
    /* 当前指令 */
	IRInst inst;
    /* 预读下一条指令 */
	IRInst ninst;

	for (inst = bb->insth.next; inst != &bb->insth; inst = inst->next) {

        /* 预读下一条指令 */
		ninst = inst->next;
		if ((inst->opcode == CALL || (inst->opcode >= EXTI1 && inst->opcode <= CVTF8U4)) && 
		    ninst->opcode == MOV && inst->opds[0] == ninst->opds[1]) {

			inst->opds[0]->ref  -= 2;
			inst->opds[0]       = ninst->opds[0];
			inst->next          = ninst->next;
			ninst->next->prev   = inst;
			bb->ninst--;
			if (ninst->opds[0]->kind == SK_Temp)
				DefineTemp (ninst->opds[0], inst->opcode, (Symbol)inst, NULL);
		} else if ((inst->opcode == MOV || inst->opcode == IMOV) && inst->opds[1]->kind == SK_Temp) {

			ValueDef    def = AsVar(inst->opds[1])->def;
			IRInst      p;

			if (def->op == MOV) {

				for ( ; def; def = def->link) {

					p = (IRInst)def->src1;
					p->opds[0]->ref--;
					inst->opds[0]->ref++;
					p->opds[0]  = inst->opds[0];
					p->opcode   = inst->opcode;
				}
				inst->opds[0]->ref--;
				inst->opds[1]->ref--;
				inst->prev->next = inst->next;
				inst->next->prev = inst->prev;
			}
		} else if ((inst->opcode == ADD || inst->opcode == SUB) && ninst->opcode == MOV &&
		         inst->opds[0] == ninst->opds[1] && inst->opds[1] == ninst->opds[0] &&
		         inst->opds[2]->kind == SK_Constant) {

			int tcode = TypeCode(inst->ty);
			if ((tcode == F4 && inst->opds[2]->val.f == 1.0f) ||
			    (tcode == F8 && inst->opds[2]->val.d == 1.0)  ||
			    (inst->opds[2]->val.i[0] == 1)) {

				inst->opds[0]->ref -= 2;
				inst->opds[1]->ref--;
				inst->opds[2]->ref--;
				inst->opcode    = inst->opcode == ADD ? INC : DEC;
				inst->opds[0]   = inst->opds[1];
				inst->opds[1]   = inst->opds[2] = NULL;
				inst->next      = ninst->next;
				ninst->next->prev = inst;
			}
		}
	}
}

/* 消除无用代码
 * 在中间代码的生成过程中,会产生一些从未被使用的临时变量定义,
 * 无用代码消除就是找到这些未被使用的临时变量,删除对应的指令。*/
static void EliminateCode (BBlock bb)
{
	IRInst  inst = bb->insth.next;
	IRInst  ninst;
	int     found = 0;
	Symbol  *opds;

find_unused_temp:
	for ( ; inst != &bb->insth; inst = inst->next) {

		ninst   = inst->next;
		opds    = inst->opds;

		if (inst->opcode == CALL) {

            /* 删除函数调用生成的中间变量 */
			if (opds[0] && opds[0]->kind == SK_Temp && opds[0]->ref == 1) {

				opds[0]->ref = 0;
				opds[0] = NULL;
			}
		} else if (inst->opcode <= BCOM || (inst->opcode >= ADDR && inst->opcode <= MOV)) {

            /* 算数运算, &, *, 强制类型转换, = 生成的临时变量 */
			if (opds[0]->kind == SK_Temp && opds[0]->ref == 1) {

				opds[0]->ref = 0;
				opds[1]->ref--;
				if (opds[2]) opds[2]->ref--;
                /* 删除临时赋值操作 */
				inst->prev->next = inst->next;
				inst->next->prev = inst->prev;
				found = 1;
				bb->ninst--;
			}
		}
	}

	if ( found ) {

		found = 0;
		inst = bb->insth.next;
		goto find_unused_temp;
	}

	return;
}


/* 优化 */
void Optimize (FunctionSymbol fsym)
{
	BBlock bb;

	for (bb = fsym->entryBB; bb; bb = bb->next) {

        /* 窥孔优化 */
		PeepHole (bb);
	}

	for (bb = fsym->entryBB; bb; bb = bb->next) {

        /* 消除无用代码 */
		EliminateCode (bb);
        /* 检查跳转 */
		ExamineJump (bb);
	}

	for (bb = fsym->entryBB; bb; ) {

        /* 合并块 */
		bb = TryMergeBBlock (bb, bb->next);
	}
}

