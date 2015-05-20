/*************************************************************************
	> File Name: vector.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月17日 星期一 21时37分41秒

	> Description: 集合的操作函数
 ************************************************************************/

#include "pcl.h"

/* 创建一个集合用所给的大小 */
Vector CreateVector (int size) 
{
    Vector v = NULL;

    ALLOC (v);
    /* 初始化 */
    v->data = (void**)HeapAllocate (CurrentHeap, size * sizeof (void*));
    v->size = size;
    v->len = 0;
    return v;
}

/* 给集合扩容，为原来的两倍 */
void ExpandVector (Vector v) 
{
    void *orig;

    if (NULL == v) {

        v = CreateVector (1);
        return ;
    }

    orig = v->data;
    v->data = (void**)HeapAllocate (CurrentHeap, 2 * v->size * sizeof (void*));
    memcpy (v->data, orig, v->size * sizeof (void*));
    v->size *= 2;
}
