/*************************************************************************
	> File Name: output.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年02月08日 星期日 12时20分51秒

	> Description: 
 ************************************************************************/

#include "ego.h"

void displayProbab (void)
{
    int i, j;
    WordBucket      word;
    SuccCharProbab  succ;

    for (i = 0; i <= NAME_HASH_MASK; i++) {

        word = WordBuckets[i];
        while ( word ) {

            printf ("%s:\n", word->word);
            for (succ = word->succHead; succ; succ = succ->succ) {

                printf ("\t %s: %d \n", succ->word, succ->count);
            }
            word = word->link;
        }
    }
}
