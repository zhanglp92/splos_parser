/*************************************************************************
	> File Name: alloc.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月31日 星期三 15时35分44秒

	> Description: 内存管理的实现
 ************************************************************************/

#include "pcl.h"

/* 管理空闲内存块的堆 */
static HEAP (FHeap);
/* 内存管理中使用到的字节对齐基数 */
static unsigned char ALIGN_BASE = sizeof (union align);

/* 初始化堆hp */
void InitHeap (Heap hp) 
{
    /* 初始化链表的头部 */
    hp->head.next   = NULL;
    hp->head.begin  = hp->head.end = hp->head.current = NULL;
    /* 链表的尾指向链表的头部 */
    hp->last = &hp->head;
}

/* 在堆hp 中申请size 大小的空间 */
void* HeapAllocate (Heap hp, int size) 
{
    struct mblock *blk = NULL, *fblk = NULL;

    /* 重置size 按ALIGN_BASE 基数对齐 */
    size = ALIGN (size, ALIGN_BASE);

    /* 从堆的每一个块中遍历查找适合的块 */
    for (blk = (&hp->head)->next; blk; blk = blk->next) {

        if (size <= blk->end - blk->current) 
            goto alloc;
    }

    /* 遍历空闲内存块(相当于新申请的块) */
    for (blk = hp->last, fblk = (&FHeap.head); 
         (blk->next = fblk->next); fblk = fblk->next) {

        if (size <= blk->next->end - blk->next->current) {

            /* 当前块空间合适，则取出此块加入到hp中 */
            blk = blk->next;
            if (!(fblk->next = blk->next)) 
                FHeap.last = fblk;
            goto newAlloc;
        }
    }

    /* 申请新的内存块 */
    int m = size + MBLOCK_SIZE + sizeof (struct mblock);

    /* 此时的blk 指向h　的最后一个存储块 */
    blk->next = malloc (m);
    if (!(blk = blk->next)) {

        Fatal ("Memory exhausted");
    }
    blk->end = (char*)blk + m;
    /* +1 表示跳过blk自身的大小 */
    blk->begin = blk->current = (char*)(blk+1);

newAlloc:
    blk->next  = NULL;
    hp->last   = blk;

alloc:
    /* size 大小已经分配 */
    blk->current += size;
    /* 返回分配的块的起始地址 */
    return blk->current - size;
}

/* 释放堆hp */
void FreeHeap (Heap hp) 
{
    struct mblock *blk = hp->head.next;
    for (blk; blk; blk = blk->next) {
    
        /* 堆中内存块初始化 */
        blk->current = blk->begin;
    }
    /* 加入到空闲堆中 */
    FHeap.last->next = hp->head.next;
    FHeap.last = hp->last;
    FHeap.last->next = NULL;

    InitHeap (hp);
}
