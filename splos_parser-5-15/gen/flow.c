/*************************************************************************
	> File Name: flow.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年01月27日 星期二 14时40分55秒

	> Description: 中间代码控制流优化
 ************************************************************************/

#include "pcl.h"
#include "gen.h"

/* 给bb 添加一个前驱p  */
static void AddPredecessor (BBlock bb, BBlock p)
{
	CFGEdge e;

	ALLOC(e);
	e->bb = p;
    /* 头插法给bb 插入一个前驱 */
	e->next = bb->preds;
	bb->preds = e;
	bb->npred++;
}

/* 给bb 添加一个后继s */
static void AddSuccessor (BBlock bb, BBlock s)
{
	CFGEdge e;

	CALLOC(e);
	e->bb   = s;
    /* 头插法给bb 插入一个后继 */
	e->next = bb->succs;
	bb->succs = e;
	bb->nsucc++;
}

/* 添加后继和前驱 */
void DrawCFGEdge (BBlock head, BBlock tail)
{
    AddSuccessor (head, tail);
    AddPredecessor (tail, head);
}

/* 删除一个基本块 */
static void RemoveEdge (CFGEdge *pprev, BBlock bb)
{
	CFGEdge e;

	for (e = *pprev; e; e = e->next) {

		if (e->bb == bb) {

			*pprev = e->next;
			return;
		}
		pprev = &e->next;
	}
	assert(0);
}


/* 移除一个前驱 */
static void RemovePredecessor (BBlock bb, BBlock p)
{
	RemoveEdge (&bb->preds, p);
	bb->npred--;
}

/* 移除一个后继 */
static void RemoveSuccessor (BBlock bb, BBlock s)
{
	RemoveEdge (&bb->succs, s);
	bb->nsucc--;
}

/* 检查跳转 */
void ExamineJump (BBlock bb)
{
	IRInst  lasti;
	CFGEdge succ;
	BBlock  bb1, bb2;

	lasti = bb->insth.prev;
	if (!(lasti->opcode >= JZ && lasti->opcode <= JMP))
		return;

	/**
	 * jump to jump             conditional jump to jump
	 *
	 * jmp bb1                  if a < b jmp bb1
	 * ...                      ...
	 * bb1: jmp bb2             bb1: jmp bb2
	 *
	 */

    /* 查找要跳转到的目标块是否在后继列表中存在 */
    for (succ = bb->succs; succ; succ = succ->next) {

        if (succ->bb == (BBlock)lasti->opds[0])
            break;
    }


    /*
	succ = bb->succs;
	do {

		if (succ->bb == (BBlock)lasti->opds[0])
			break;
		succ = succ->next;
	} while (succ != NULL);
    */

    /* 找不到块则,异常退出 */
    assert (succ != NULL);
    

    /* 找到了要跳转的目标块, 
     * 检查目标块中的指令, 如果只有一个跳转指令,
     * 则,优化 */
	bb1 = succ->bb;
	if (bb1->ninst == 1 && bb1->insth.prev->opcode == JMP) {

		bb2 = bb1->succs->bb;
		succ->bb = bb2;
		RemovePredecessor (bb1, bb);
		AddPredecessor (bb2, bb);
		bb1->ref--;
		bb2->ref++;
		lasti->opds[0] = (Symbol)bb2;
	}
}

/* 尝试添加一个前驱 */
static void TryAddPredecessor (BBlock bb, BBlock p)
{
	CFGEdge pred;

	for (pred = bb->preds; pred; pred = pred->next) {

		if (pred->bb == p)
			return;
	}

	AddPredecessor (bb, p);
}

/* 尝试添加一个后继 */
static void TryAddSuccessor (BBlock bb, BBlock s)
{
	CFGEdge succ;

	for (succ = bb->succs; succ; succ = succ->next) {

		if (succ->bb == s)
			return;
	}
	AddSuccessor (bb, s);
}

/* 合并两个基本块里的三地址码到bb1 中 */
static void MergeInstructions (BBlock bb1, BBlock bb2)
{
	IRInst bb1_lasti    = bb1->insth.prev;
	IRInst bb2_firsti   = bb2->insth.next;
	
	if (bb1_lasti->opcode >= JZ && bb1_lasti->opcode <= JMP) {

		bb1_lasti = bb1_lasti->prev;
		bb1->ninst--;
	}

	if (bb2->ninst == 0) {

		bb1->insth.prev = bb1_lasti;
		bb1_lasti->next = &bb1->insth;
	} else {

		bb1_lasti->next = bb2_firsti;
		bb2_firsti->prev = bb1_lasti;
		bb2->insth.prev->next = &bb1->insth;
		bb1->insth.prev = bb2->insth.prev;
	}
	bb1->ninst += bb2->ninst;
}

/* 修改后继,去掉块s 修改将bb 的后继和跳转,修改ns 的前驱 */
static void ModifySuccessor (BBlock bb, BBlock s, BBlock ns)
{
	IRInst lasti = bb->insth.prev;

    /* 从bb 后继中删除s */
	RemoveSuccessor (bb, s);
    /* 将ns 添加到bb 的后继中 */
	TryAddSuccessor (bb, ns);
    /* 将bb 添加到ns 的前驱中 */
	TryAddPredecessor (ns, bb);

    /* 检查bb 块中的跳转 */
	if (lasti->opcode >= JZ && lasti->opcode <= JMP) {

        /* 如果目标块是s 则,指向ns */
		if ((BBlock)lasti->opds[0] == s) {

			s->ref--;
			ns->ref++;
			lasti->opds[0] = (Symbol)ns;
		}
	} else if (lasti->opcode == IJMP) {

        /* 修改间接跳转 */
		BBlock *dstBBs = (BBlock *)lasti->opds[0];
		int i = 0;

		for ( ; dstBBs[i]; i++) {

			if (dstBBs[i] == s) {

				s->ref--;
				ns->ref++;
				dstBBs[i] = ns;
			}
		}
	}
}


/* 尝试合并两个块 */
BBlock TryMergeBBlock(BBlock bb1, BBlock bb2)
{
	if (bb2 == NULL)
		return bb2;

    /* 如果两个都是并行块,则必然是同一个并行模块中的块 */
    if (bb1->isParall ^ bb2->isParall)
        return bb2;


    /* bb1只有一个后继切是bb2, bb2只有一个前驱 */
	if (bb1->nsucc == 1 && bb2->npred == 1 && bb1->succs->bb == bb2) {

		CFGEdge succ;

        /* 若bb2 后继中块的前驱, 包含bb2 块,
         * 则在此块中删除bb2, 重新加入bb1 */
		for (succ = bb2->succs; succ; succ = succ->next) {

			RemovePredecessor (succ->bb, bb2);
			AddPredecessor (succ->bb, bb1);
		}

        /* bb1 后继重新指向为bb2 的后继 */
		bb1->succs = bb2->succs;
		bb1->nsucc = bb2->nsucc;

        /* 合并bb1 和bb2 的三地址码到,bb1 中 */
		MergeInstructions (bb1, bb2);
        /* 去掉bb2 */
		bb1->next = bb2->next;
		if (bb2->next)
			bb2->next->prev = bb1;

		return bb1;
	} else if (bb1->ninst == 0) {

        /* bb1 块中没有三地址码 */
		CFGEdge pred;

        /* 从bb2 前驱中删除bb1 */
		RemovePredecessor (bb2, bb1);


		for (pred = bb1->preds; pred; pred = pred->next) {

            /* 去掉bb1, 修改和bb1 的关系 */
			ModifySuccessor (pred->bb, bb1, bb2);
		}

        /* 去掉块bb1 重新链接 */
		bb2->prev = bb1->prev;
		if (bb1->prev)
			bb1->prev->next = bb2;
		
		return bb1->prev;
	} else if (bb2->ninst == 0 && bb2->npred == 0) {

        /* bb2 中没有三地址码, 并且bb2 没有前驱 */
		if (bb2->next == NULL) {

			bb1->next = NULL;
		} else {

			RemovePredecessor (bb2->next, bb2);
            /* 去掉bb2, 重新链接 */
			bb1->next = bb2->next;
			bb2->next->prev = bb1;
		}

		return bb1;
	} else if (bb2->ninst == 0 && bb2->npred == 1 && bb2->preds->bb == bb1) {

        /* bb2 中没有三地址码, bb2只有一个前驱是bb1 */
		if (bb2->next == NULL) {

			bb1->next = NULL;
		} else {

			RemovePredecessor (bb2->next, bb2);
			ModifySuccessor (bb1, bb2, bb2->next);
			bb1->next = bb2->next;
			bb2->next->prev = bb1;
		}

		return bb1;
	} else {

		IRInst lasti = bb1->insth.prev;
		if (lasti->opcode == JMP && bb1->succs->bb == bb1->next) {

			lasti->prev->next = lasti->next;
			lasti->next->prev = lasti->prev;
			bb1->ninst--;
			return bb1;
		}
	}
	
	return bb2;
}
