/*************************************************************************
	> File Name: simp.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年02月08日 星期日 13时32分00秒

	> Description: 
 ************************************************************************/

#include "ego.h"


static void display (SuccCharProbab succ)
{
    SuccCharProbab p = succ;

    while ( p ) {

        printf ("%s (%d) --> ", p->word, p->count);
        p = p->succ;
    }
    printf ("\n\n");
}

/* 给后继排序 */
static void WordSort (SuccCharProbab *succ)
{
    struct succCharProbab   head;
    SuccCharProbab  pre1, pre2;
    SuccCharProbab  p, q, max, t;

    /* 构造一个头部 */
    head.succ = *succ;

    for (pre1 = &head; pre1->succ; pre1 = pre1->succ) {

        p = pre1->succ;
        max = pre1;
        for (pre2 = pre1->succ; pre2->succ; pre2 = pre2->succ) {

            q = pre2->succ;
            if (q->count > max->succ->count) 
                max = pre2;
        }

        if (max->succ != p) {

            /* 调整pre2 和max 的位置 */
            pre2 = max;
            max = max->succ;

            /* 交换后继 */
            pre1->succ = max;
            pre2->succ = p;

            t = p->succ;
            p->succ = max->succ;
            max->succ = t;
        }
    }

    *succ = head.succ;
}

/* 给一个word 的所有后继排序 */
static void SuccSort (WordBucket word)
{
    int     i;
    SuccCharProbab  succ;
    SuccCharProbab  *tail = &word->succHead;

    if (word == NULL)
        return ;

    for (i = 0; i <= NAME_HASH_MASK; i++) {

        succ = word->succs[i];
        while ( succ ) {

            *tail = succ;
            tail = &(*tail)->succ;
            succ = succ->link;
        }
    }

    WordSort (&word->succHead);
}

void SimpSort (void) 
{
    int i, j;
    WordBucket      word;
    SuccCharProbab  succ;

    for (i = 0; i <= NAME_HASH_MASK; i++) {

        word = WordBuckets[i]; 
        while ( word ) {

            SuccSort (word);
            word = word->link;
        }
    }
}
