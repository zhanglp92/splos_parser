/*************************************************************************
	> File Name: output.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 22时13分25秒

	> Description: 
 ************************************************************************/

#include "pcl.h"

#define BUF_LEN     4096

char OutBuffer[BUF_LEN];
int BufferSize;

void Flush (void)
{
    if ( BufferSize ) {

        fwrite (OutBuffer, 1, BufferSize, ASMFile);
        BufferSize = 0;
    }
}

void PutString (const char *s) 
{
    int i, len = strlen (s);
    char *p;

    if (BUF_LEN < len) {
    
        fwrite (s, 1, len, ASMFile);
        return ;
    } 

    if (BUF_LEN - BufferSize < len) 
        Flush ();
    p = OutBuffer + BufferSize;
    strncpy (OutBuffer + BufferSize, s, len);
    BufferSize += len;
}

/* 左对齐 */
void LeftAlign (FILE *file, int pos)
{
    char spaces[256];

    if (256 <= pos) pos = 2;
    memset (spaces, ' ', pos);
    spaces[pos] = 0;

    if (file != ASMFile) {
        /*  */
        fprintf (file, "\n%s", spaces);
    } else {
        /* 填asm 文件 */
        PutString (spaces);
    }
}

/* 将格式化到buf 中,并将buf 加入到字符串池中,返回buf */
char* FormatName (const char *fmt, ...)
{
    char buf[256];
    va_list ap;

    /* 使用可变参,调用vsprintf 函数 */
    va_start (ap, fmt);
    vsprintf (buf, fmt, ap);
    va_end (ap);

    /* 将buf加入到字符串池中,返回buf */
    return InternStr (buf, strlen (buf));
}

/* 用参数构造一个文件名,并且以写的方式打开 */
FILE* CreateOutput (const char *filename, const char *ext)
{
    char tmp[256] = {};
    char *p = tmp;

    while (*filename && *filename != '.')
        *p++ = *filename++;
    strcpy (p, ext);

    return fopen (tmp, "w");
}
