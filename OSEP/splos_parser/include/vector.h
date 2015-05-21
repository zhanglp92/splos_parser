/*************************************************************************
	> File Name: vector.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 16时09分58秒

	> Description: 
 ************************************************************************/

#ifndef __VECTOR_H
#define __VECTOR_H

/************************************************************************
 * 数据结构的设计
 ************************************************************************/
/* 自定义集合类型 */
typedef struct vector {

    /* 集合的数据 */
    void **data;
    /* 集合的长度 */
    int len;
    /* 集合的大小 */
    int size;
} *Vector;

#define LEN(v) ((v)->len)

/* 得到第i 个元素 */
#define GET_ITEM(v, i) ((v)->data[(i)])
/* 去集合顶部元素 */
#define TOP_ITEM(v)    (v->len == 0 ? NULL : v->data[v->len - 1])

/* 向集合中插入一个元素 */
#define INSERT_ITEM(v, item)        \
    do {                            \
        if ((v)->len >= (v)->size)  \
            ExpandVector (v);       \
        (v)->data[(v)->len++] = item;   \
    } while (0)

#define POP_ITEM(v) (v->len == 0 ? NULL : v->data[--(v->len)])

/* 遍历vector,for 的头部 */
#define FOR_EACH_ITEM(ty, item, v)      \
    {                                   \
        int i = 0;                      \
        for (i = 0; i < v->len; i++) {  \
            item = (ty)v->data[i];      
/* for 的结尾 */
#define ENDFOR \
        }       \
    }

/* 打印vector 里边的内容 */
#define printVector(v, ty, form)    \
    {                               \
        if (v) {                    \
            ty item;                    \
            FOR_EACH_ITEM (ty, item, v) \
                printf (form, item);    \
            ENDFOR                  \
        }                           \
    }

/************************************************************************
 * 接口　
 ************************************************************************/
/* 创建一个集合 */
Vector CreateVector (int size);
/* 给集合扩容，为原来的两倍 */
void ExpandVector (Vector v);

#endif
