/*************************************************************************
	> File Name: str.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 10时44分38秒

	> Description: 
 ************************************************************************/

#ifndef __STR_H
#define __STR_H

/************************************************
 * 数据结构
 ************************************************/

/* 存储字符串的数据结构 */
typedef struct nameBucket {

    /* 字符串的内容 */
    char *name;
    /* 字符串长度 */
    int len;
    /* 同一组字符串链表 */
    struct nameBucket *link;
} *NameBucket;


/************************************************
 * 函数接口 
 ************************************************/

/* 处理字符串的哈希函数 */
unsigned int ELFHash (const char *str, int len);
/* 将一个字符串加入到字符串池中 */
char* InternStr (const char *id, int len);

#endif
