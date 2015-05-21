/*************************************************************************
	> File Name: file.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2015年01月19日 星期一 14时16分24秒

	> Description: 
 ************************************************************************/

//#import splos_string.p;


#import { 
    
    ./test/test1.h;
    ./test/test.h;
}


//import ./test/test3.c;

#define TestDefine 10

#define dd fdsafsfad \
        fdssfds \
        fdsaaa  \
        fsdfasdd \
        fdsfdsaf \

#undef dd

#define dd

static void fun1 (void)
{
    int     b = 0;
    int     a = 10;

#define test int c = 2
    test ;

#undef  test

#ifndef test

#include "test/test1.h"

#endif

    c = 10;
    aa[1][1] = 8;

    parallel {
    
        a = 3;
    }


/*
    parallel {
        
        b = 10;
        if (b > 10) {
            
            printf ("10");
        } else {

            printf ("0");
        }
    }

    switch (a) {
        
        case 0: {

            a += 10;
        }break;

        case 10: {

            a++;
        }break;

        case 3: {
            
            a--;
        }break;

        default:
            a +=3;
    }

    if (0) {

    }else {

    }
    */
}

/*
void fun (int a, int b)
{
    b += 10;
    a += 10;
}

struct A {

    int     a;
    float   b;
};

typedef int B;

int BB;

double CC;

void fun2 (void) 
{
    int aa;

    aa += 1;

    switch (aa) {
        
        case 1:
            printf ("1");
            break;

        default:
            printf ("error");
            break;
    }

table:
    return ;
}

int main (void)
{

    struct A    aa, b;
    int i;

    open ();

    if (i == 0) {
    
        int a;
        printf ("a = %d \n", a);
    } else {

        int a;
        printf ("a = %d \n", a);
    }

    return 0;
} */
