/*************************************************************************
	> File Name: gen.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月23日 星期二 22时52分46秒

	> Description: 
 ************************************************************************/

#include "pcl.h"
#include "gen.h"

#define IsCommute(op) 0

BBlock CurrentBB;
int isParall = 0;


int OPMap[] =  {

    #define OPINFO(tok, prec, name, func, opcode) opcode,
    #include "opinfo.h"
    #undef  OPINFO
};


/* 创建一个基本块 */
BBlock CreateBBlock (void)
{
    static int no = 1;

	BBlock bb;

	CALLOC(bb);
    /* 设置是否为并行块 */
    bb->isParall = isParall;
    bb->no = no++;

    /* 指令的操作码 */
	bb->insth.opcode = NOP;
    /* 上一条, 下一条指令都指向自己 */
	bb->insth.prev = bb->insth.next = &bb->insth;

	return bb;
}

/* 将bb 设置成当前块 */
void StartBBlock (BBlock bb)
{
	IRInst lasti;

    /* 链接到基本块 */
	CurrentBB->next = bb;
	bb->prev = CurrentBB;

    /* 当前基本块的上一条指令 */
	lasti = CurrentBB->insth.prev;
	if (lasti->opcode != JMP && lasti->opcode != IJMP) {

		DrawCFGEdge (CurrentBB, bb);
	}
    /* 改变当前块的位置 */
	CurrentBB = bb;
}

/* 向当前基本块中加入一条三地址码 */
void AppendInst (IRInst inst)
{
	assert (CurrentBB != NULL);

	CurrentBB->insth.prev->next = inst;
	inst->prev = CurrentBB->insth.prev;

	inst->next = &CurrentBB->insth;
	CurrentBB->insth.prev = inst;

    /* 基本块中指令的数目 */
	CurrentBB->ninst++;
}

/* 填写基本块dstBB 的内容, 并追加到当前块中 */
/* 
 * BranchInst   -> if Operand RelOper Operand goto Label |
 *              -> if Operand goto Label |
 *              -> if !Operand goto Label 
 * */
void GenerateBranch (Type ty, BBlock dstBB, int opcode, Symbol src1, Symbol src2)
{
	IRInst inst;

	CALLOC(inst);
	dstBB->ref++;
	src1->ref++;
	if (src2) src2->ref++;

    /* 把dstBB 块添加到CurrentBB 中 */
	DrawCFGEdge (CurrentBB, dstBB);
	inst->ty        = ty;
	inst->opcode    = opcode;
	inst->opds[0]   = (Symbol)dstBB;
	inst->opds[1]   = src1;
	inst->opds[2]   = src2;
	AppendInst (inst);
}

/* 将值def, 添加到符号p 中 */
static void TrackValueUse (Symbol p, ValueDef def)
{
	ValueUse use;

	CALLOC(use);
	use->def    = def;
	use->next   = AsVar(p)->uses;
	AsVar(p)->uses = use;
}

/* 构造表达式的值,并将其添加到 t,src1,src2三个变量的uses 中 */
void DefineTemp (Symbol t, int op, Symbol src1, Symbol src2)
{
	ValueDef    def;

	CALLOC(def);
	def->dst    = t;
	def->op     = op;
	def->src1   = src1;
	def->src2   = src2;
	def->ownBB  = CurrentBB;

    /* 赋值和函数调用 */
	if (op == MOV || op == CALL) {

        /* 将值添加到变量t 中 */
		def->link       = AsVar(t)->def;
		AsVar(t)->def   = def;
		return;
	}

    /* 如果src1 和src2是变量则将值添加进去 */
	if (src1->kind == SK_Variable) {

		TrackValueUse (src1, def);
	}

	if (src2 && src2->kind == SK_Variable) {

		TrackValueUse (src2, def);
	}
	AsVar(t)->def = def;
}


/* 构造一个三地址码, 并添加到当前块中 */
/* 
 * AssignInst   -> VarName = Expression |
 *              -> VarName = Operand |
 *              -> *VarName = Operand
 * */
void GenerateAssign (Type ty, Symbol dst, int opcode, Symbol src1, Symbol src2)
{
	IRInst inst;

	CALLOC(inst);
    /* 符号被引用一次 */
	dst->ref++;
	src1->ref++;
	if (src2) src2->ref++;

    /* 构造三地址码 */
	inst->ty      = ty;
	inst->opcode  = opcode;
	inst->opds[0] = dst;
	inst->opds[1] = src1;
	inst->opds[2] = src2;

    /* 将三地址码添加到当前基本块中 */
	AppendInst (inst);

    /* 构造表达式值,并添加到dst,src1,src2的uses中 */
	DefineTemp (dst, opcode, src1, src2);
}

/* 生成间接跳转指令 */
/*
 * IJumpInst    -> goto (label1, label2, ...)[VarName]
 * */
void GenerateIndirectJump (BBlock *dstBBs, int len, Symbol index)
{
	IRInst  inst;
	int     i;
	
	CALLOC(inst);
	index->ref++;
    /* 将每个块都加入当前块中 */
	for (i = 0; i < len; ++i){

		dstBBs[i]->ref++;
		DrawCFGEdge (CurrentBB, dstBBs[i]);
	}

	inst->ty      = T(VOID);
	inst->opcode  = IJMP;
    /* 要跳转的目标块数组 */
	inst->opds[0] = (Symbol)dstBBs;
    /* 数组的下标 */
	inst->opds[1] = index;
	inst->opds[2] = NULL;
	AppendInst (inst);
}

/* 生成直接跳转指令 */
/*
 * JumpInst     -> goto Label
 * */
void GenerateJump (BBlock dstBB)
{
	IRInst inst;

	CALLOC(inst);
	dstBB->ref++;
    /* 要跳转到的块,dstBB 加入当前块 */
	DrawCFGEdge (CurrentBB, dstBB);
	inst->ty      = T(VOID);
	inst->opcode  = JMP;
	inst->opds[0] = (Symbol)dstBB;
	inst->opds[1] = inst->opds[2] = NULL;
	AppendInst (inst);
}

/* 生成return 指令 */
/* 
 * ReturnInst   -> return VarName
 * */
void GenerateReturn (Type ty, Symbol src)
{
	IRInst inst;

	CALLOC(inst);
	src->ref++;
	inst->ty      = ty;
	inst->opcode  = RET;
	inst->opds[0] = src;
	inst->opds[1] = inst->opds[2] = NULL;
	AppendInst (inst);
}

/* 
 * ClearInst    -> Clear VarName, constant
 * */
void GenerateClear (Symbol dst, int size)
{
	IRInst inst;

	CALLOC (inst);
	dst->ref++;
	inst->ty      = T(UCHAR);
	inst->opcode  = CLR;
	inst->opds[0] = dst;
	inst->opds[1] = IntConstant (size);
	inst->opds[1]->ref++;
	inst->opds[2] = NULL;
	AppendInst (inst);
}

/* 操作为非取地址时,目标值置空 */
static void TrackValueChange (Symbol p)
{
	ValueUse use = AsVar(p)->uses;

	for ( ; use; use = use->next) {

        /* 操作为非取地址时,目标值置空 */
		if (use->def->op != ADDR)
			use->def->dst = NULL;
	}
}

/* 生成赋值指令 */
void GenerateMove (Type ty, Symbol dst, Symbol src)
{
	IRInst inst;

	ALLOC(inst);
	dst->ref++;
	src->ref++;

	inst->ty = ty;
	inst->opcode  = MOV;
	inst->opds[0] = dst;
	inst->opds[1] = src;
	inst->opds[2] = NULL;
	AppendInst (inst);

	if (dst->kind == SK_Variable) {

		TrackValueChange (dst);
	} else if (dst->kind == SK_Temp) {

		DefineTemp (dst, MOV, (Symbol)inst, NULL);
	}
}

/* 尝试添加操作的值
 * 在函数符号表中查找该操作, 
 * 如果找到复合条件的则返回目标符号, 
 * 否则创建值并添加到符号表中 */
Symbol TryAddValue (Type ty, int op, Symbol src1, Symbol src2)
{
	int         h = ((unsigned long long)src1 + (unsigned long long)src2 + op) & 15;
	ValueDef    def;
	Symbol      t;

    /* 非地址符号 */
	if (op != ADDR && (src1->addressed || (src2 && src2->addressed)))
		goto new_temp;

    /* 在函数的符号表中查找此操作的值 */
	for (def = FSYM->valNumTable[h]; def; def = def->link) {

		if (def->op == op && (def->src1 == src1 && def->src2 == src2))
			break;
	}

    /* 如果查找到该值,并属于当前基本块,并且有目标符号,则直接返回,
     * 否则创建中间变量 */
	if (def && def->ownBB == CurrentBB && def->dst != NULL)
		return def->dst;

/* 创建新临时变量 */
new_temp:
    /* 创建临时变量添加到函数符号表中 */
	t = CreateTemp (ty);
    /* 构造一个三地址码,并添加到当前块中 */
	GenerateAssign (ty, t, op, src1, src2);

    /* t->def 是在GenerateAssign 中构造的值 */
	def       = AsVar(t)->def;
    /* 添加值到符号表中 */
	def->link = FSYM->valNumTable[h];
	FSYM->valNumTable[h] = def;
	return t;
} 

/* 间接赋值(解址后赋值: a = *b ) */
void GenerateIndirectMove (Type ty, Symbol dst, Symbol src)
{
	IRInst inst;

	CALLOC(inst);
	dst->ref++;
	src->ref++;

	inst->ty      = ty;
	inst->opcode  = IMOV;
	inst->opds[0] = dst;
	inst->opds[1] = src;
	inst->opds[2] = NULL;
	AppendInst (inst);
}

/* 取址运算 */
Symbol Deref (Type ty, Symbol addr)
{
	Symbol tmp;
	
    /* 取址运算 */
	if (addr->kind == SK_Temp && AsVar(addr)->def->op == ADDR) {

		return AsVar(addr)->def->src1;
	}

	tmp = CreateTemp (ty);
	GenerateAssign (ty, tmp, DEREF, addr, NULL);
	return tmp;
}

/* 构造地址
 * 如果符号p 是临时变量并且是取地址操作则,返回操作数1 
 * 否则构造值,并添加值 */
Symbol AddressOf (Symbol p)
{
	if (p->kind == SK_Temp && AsVar(p)->def->op == DEREF) {

		return AsVar(p)->def->src1;
	}

    /* 标记次符号为地址 */
	p->addressed = 1;
	if (p->kind == SK_Variable) {

		TrackValueChange (p);
	}

    /* 添加指针操作的值 */
	return TryAddValue (T(POINTER), ADDR, p, NULL); 
}

/* 函数调用 */
/* 
 * CallInst     -> [VarName =] call VarName(Operand, Operand, ...)
 * */
void GenerateFunctionCall (Type ty, Symbol recv, Symbol faddr, Vector args, int callNum)
{
	ILArg   p;
	IRInst  inst;

	CALLOC(inst);
	if (recv) 
        recv->ref++;
	faddr->ref++;

	FOR_EACH_ITEM (ILArg, p, args)
		p->sym->ref++;
	ENDFOR

	inst->ty      = ty;
	inst->opcode  = CALL;
    inst->callNum = callNum;
    /* 返回值 */
	inst->opds[0] = recv;
	inst->opds[1] = faddr;
    /* 参数 */
	inst->opds[2] = (Symbol)args;
	AppendInst (inst);

    /* 给返回值创建临时变量 */
	if (recv != NULL)
		DefineTemp (recv, CALL, (Symbol)inst, NULL);
}
