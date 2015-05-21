/*************************************************************************
	> File Name: input.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 19时50分12秒

	> Description: 
 ************************************************************************/

#ifndef __INPUT_H
#define __INPUT_H

/* 文件加载到内存后的属性 */
struct input {

    /* 文件名 */
    const char *filename;
    /* 将全部文件装载到内存后的起始地址 */
    unsigned char *base;
    /* 文件的当前位置 */
    unsigned char *current;
    /* 文件描述符 */
    void *file;
    /* win32中映射时使用 */
    void *fileMapping;
    /* 文件内容的总长度 */
    unsigned long long size;
};


/**********************************************
 * 接口
 **********************************************/
void ReadSourceFile (const char *filename);
void CloseSourceFile (void);

/* 文件结束标志 */
extern unsigned char END_OF_FILE;
/* 保存打开的源文件的信息 */
extern struct input Input;

#endif
