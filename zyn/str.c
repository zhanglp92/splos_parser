/*************************************************************************
	> File Name: str.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 10时25分37秒

	> Description: 字符串和标示符的处理
 ************************************************************************/

#include "ego.h"

static NameBucket NameBuckets[NAME_HASH_MASK + 1];

/* 处理字符串的哈希函数 */
unsigned int ELFHash (const char *str, int len)
{
    unsigned int h = 0, x = 0;
    int i;

    for (i = 0; i < len; i++) {

        h = (h << 4) + *str++;
        if ((x = h & 0xF0000000)) {

            h = (h ^ (x >> 24)) & ~x;
        }
    }
    return h;
}

/* 将一个字符串加入到字符串池中 */
char* InternStr (const char *str, int len) 
{
    int i, h;
    NameBucket p;

    /* 得到哈希值 */
    h = ELFHash (str, len) & NAME_HASH_MASK;
    /* 在对应的一组链表中查找 */
    for (p = NameBuckets[h]; p; p = p->link) {

        if (len == p->len && !strncmp (str, p->name, len)) 
            return p->name;
    }
    
    p = malloc (sizeof (*p));
    p->name = malloc (len + 1);

    for (i = 0; i < len; i++) 
        p->name[i] = str[i];
    p->name[len] = 0;
    p->len = len;

    /* 将该节点尾插在对应链中 */
    p->link = NameBuckets[h];
    NameBuckets[h] = p;

    return p->name;
}
