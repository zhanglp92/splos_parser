/*************************************************************************
	> File Name: scan.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年02月07日 星期六 18时38分29秒

	> Description: 
 ************************************************************************/

#include "ego.h"

/* 当前处理的位置 */
#define Current     (Input.current)

/* 当前序列的值 */
static char *CurrentValue = NULL;

/* 处理字符序列的函数数组 */
static void (*Scanners[255])(void);

/* 单词池 */
WordBucket   WordBuckets[NAME_HASH_MASK + 1];

/* 不嫩识别的符号 */
static void ParseError (void)
{
    CurrentValue = NULL;

    if (*Current == END_OF_FILE) 
        return;

    Current += 1;
} 


/* 处理一个汉字 */
static void ParseChinese (void)
{
    int i;

    for (i = 1; i < CHINESE_B; i++) {

        if (!isChinese (*(Current + i))) {
    
            ParseError ();
            return ;
        }
    }

    CurrentValue = InternStr (Current, CHINESE_B);
    Current += CHINESE_B;
}

/* 处理一个英文单词 */
static void ParseEnglish (void)
{
    int len = 1;

    while ( isEnglish (*(Current+len)) ) 
        len++;

    CurrentValue = InternStr (Current, len);
    Current += len;
}

/* 处理其他符号 */
static void ParseSeparat (void)
{
    CurrentValue = InternStr (Current, 1);
    Current += 1;
}

/* 取下一个单词 */
static const char * GetNext (void)
{
    if (*Current == END_OF_FILE)
        return NULL;

    
    /* 滤过空白符 */
    while ( isspace (*Current) )
        Current++;

    Scanners[*Current] ();
    return CurrentValue;
}

/* 修改后继出现的概率 */
static void addSuccPro (WordBucket word)
{
    int     h;
    const char *src;
    const char *dst = word->word;
    SuccCharProbab  succ;

    /* 取得后继串 */
    if ((src = GetNext ()) == NULL) 
        return ;
    
    h = ELFHash (dst, strlen (dst)) & NAME_HASH_MASK;
    /* 遍历查找后继串 */
    for (succ = word->succs[h]; succ; succ = succ->link) {

        if (succ->word == src) {

            succ->count++;
            return ;
        }
    }

    /* 添加后继串 */
    succ = calloc (1, sizeof (*succ));
    succ->word = src;
    succ->count++;

    succ->link = word->succs[h];
    word->succs[h] = succ;
}

/* 统计概率 */
static void Probability (const char *src)
{
    int i, h;
    const char  *dst;
    WordBucket  p;


    if (src == NULL) 
        return ;

    h = ELFHash (src, strlen (src)) & NAME_HASH_MASK;

    /* 查找当前单词是否已经添加 */
    for (p = WordBuckets[h]; p; p = p->link) {

        /* 使用字符串池 */
        if (p->word == src) {
    
            /* 若找到修改后继出现的概率 */
            addSuccPro (p);
            return ;
        }
    }

    /* 添加新的单词 */
    p = calloc (1, sizeof (*p));
    p->word = src;
    /* 修改后继出现的概率 */
    addSuccPro (p);

    /* 加入到链表中 */
    p->link = WordBuckets[h];
    WordBuckets[h] = p;
}


/* 扫描文件 */
void Scanner (const char *file)
{
    ReadSourceFile (file);

    while ( *Current != END_OF_FILE ) {
    
        if (CurrentValue == NULL) 
            GetNext ();
        Probability (CurrentValue);
    }

    CloseSourceFile ();
}

void SetupEgo (void)
{
    int i;

    for (i = 0; i <= 255; i++) {

        if (isChinese (i))
            Scanners[i] = ParseChinese;
        else if (isEnglish (i)) 
            Scanners[i] = ParseEnglish;
        else if (isSeparat (i)) 
            Scanners[i] = ParseSeparat;
        else 
            Scanners[i] = ParseError;
    }
}

