/*************************************************************************
	> File Name: alloc.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年12月31日 星期三 15时02分45秒

	> Description: 为了提高pcc 的运行效率，它不是简单的使用malloc 
        和free 来完成对内存的分配和释放。由编译器所分配的很多数据
        结构具有非常重要的一个特性,就是生命期。比如说对于语法树,
        当释放根节点的时候,相应的子结点也要被释放。因此 pcc 采取
        了一个基于堆的分配策略。堆是一个可以动态增长的内存块列表。
        不同于free 操作,对于堆的释放并不真的释放内存,系统中会维护
        一个空闲内存块列表,当释放堆时,就将堆中的所有内存块放入空
        闲内存块列表中。
 ************************************************************************/

#ifndef __ALLOC_H
#define __ALLOC_H

/****************************************************
 *  数据结构的设计
 ****************************************************/
/* 内存块的描述 */
typedef struct mblock {

    /* 内存块的起始位置 */
    char *begin;
    /* 内存块的结束位置 */
    char *end;
    /* 当前可用的位置 */
    char *current;
    /* 下一个内存块 */
    struct mblock *next;
} *Mblock;

/* 堆的描述 */
typedef struct heap {

    /* 指向堆中的最后一块内存 */
    struct mblock *last;
    /* 堆链的头部(带头节点链表) */
    struct mblock head;
} *Heap;

/* 字节对齐 */
union align {
    
    double d;
    int (*f)(void);
};

/* 设置一个块的大小 */
#define MBLOCK_SIZE     (4 * 1024)
/* 给p 在CurrentHeap 堆(在ucl.c中定义)中申请一段空间 */
#define ALLOC(p)    ((p) = HeapAllocate (CurrentHeap, sizeof *(p)))
#define CALLOC(p)   memset (ALLOC(p), 0, sizeof *(p))
/* 定义并初始化一个堆 */
#define HEAP(hp)    struct heap hp = { &hp.head }

/****************************************************
 * 接口
 ****************************************************/
void  InitHeap (Heap hp);
void* HeapAllocate (Heap hp, int size);
void  FreeHeap (Heap hp);

#endif
