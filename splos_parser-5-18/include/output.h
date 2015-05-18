/*************************************************************************
	> File Name: output.h

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月14日 星期五 22时52分53秒

	> Description: 
 ************************************************************************/

#ifndef __OUTPUT_H
#define __OUTPUT_H

void fun (void);

char* FormatName (const char *fmt, ...); 
FILE* CreateOutput(const char *filename, const char *ext);
void LeftAlign(FILE *file, int pos);

#endif
