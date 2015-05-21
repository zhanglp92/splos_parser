/*************************************************************************
	> File Name: scan.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年02月07日 星期六 18时38分24秒

	> Description: 
 ************************************************************************/

#ifndef __SCAN_H
#define __SCAN_H


/* 字符序列的种类 */
enum { NK_CHINESE, NK_ENGLISH, NK_OTHER };


/* 描述后继单词出现的概率 */
typedef struct succCharProbab {

    /* 后继单词 */
    const char  *word;
    /* 出现的次数 */
    unsigned    count;
    /* hash 链 */
    struct succCharProbab *link;
    /* 给一个单词的所有后继排序 */
    struct succCharProbab *succ;
} *SuccCharProbab;


/* 描述一个单词,和所有可能的后继出现的概率 */
typedef struct curWord {
 
    /* 当前单词 */
    const char  *word;
    /* 当前单词的后继列表 */
    SuccCharProbab  succs[NAME_HASH_MASK + 1];
    /* 排序链表的头 */
    SuccCharProbab  succHead;
    /* hash 链 */
    struct curWord *link;

} *CurWord, *WordBucket;


/* 汉字 */
#define isChinese(ch)   (ch > 127 && ch < 255)
/* 英文单词 */
#define isEnglish(ch)   (isalpha (ch)) 
/* 分隔符 */
#define isSeparat(ch)   ((ch > 32 && ch < 48) || \
                         (ch > 57 && ch < 65) || \
                         (ch > 90 && ch < 97) || \
                         (ch > 122 && ch < 128))

void Scanner (const char *file);
void SetupEgo (void);

/* 单词序列 */
extern WordBucket   WordBuckets[];

#endif
