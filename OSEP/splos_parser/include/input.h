/*************************************************************************
	> File Name: input.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 19时50分12秒

	> Description: 输入文件操作
 ************************************************************************/

#ifndef __INPUT_H
#define __INPUT_H

#define FILE_NAME_LEN 31

/**********************************************
 * 数据结构
 **********************************************/
/* 表示文件中的坐标 */
typedef struct coord {

    /* 文件名 */
    char *filename;
    /* 当前行 */
    int ppline;
    /* 文件中的行,列 */
    int line;
    int col;
} *Coord;

/* 文件加载到内存后的属性 */
struct input {

    /* 文件名 */
    char *filename;
    /* 将全部文件装载到内存后的起始地址 */
    unsigned char *base;
    /* 文件的当前位置 */
    unsigned char *current;
    /* 当前行的起始地址 */
    unsigned char *lineHead;
    /* 文件的当前行 */
    int line;
    /* 文件描述符 */
    void *file;
    /* win32中映射时使用 */
    void *fileMapping;
    /* 文件内容的总长度 */
    unsigned long size;
    /* 用于标记import 的有花括号 */
    unsigned long importRbrace;
};


/**********************************************
 * 接口
 **********************************************/
long MyCreateFile (char *filename);
void MyClose (long fno);
void ReadSourceFile (char *filename);
void CloseSourceFile (void);
int FileExist (char *filename);

/* 文件结束标志 */
extern unsigned char END_OF_FILE;
/* 保存打开的源文件的信息 */
extern struct input Input;

#endif
