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
/* 字符串或标示符的hash 值掩码 */
#define NAME_HASH_MASK  1023

/* 存储字符串的数据结构 */
typedef struct nameBucket {

    /* 字符串的内容 */
    char *name;
    /* 字符串长度 */
    int len;
    /* 同一组字符串链表 */
    struct nameBucket *link;
} *NameBucket;

/* 自定义String 类型 */
typedef struct string {
    
    /* String 的内容 */
    char *chs;
    /* 字符串的长度 */
    int len;
} *String;

/************************************************
 * 函数接口 
 ************************************************/
/* 将一个字符串加入到字符串池中 */
char* InternStr (const char *id, int len);
/* 将tmp 追加到str 后边 */
void AppendSTR (String str, const char *tmp, int len, int wide);

#endif
