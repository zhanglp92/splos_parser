/*************************************************************************
	> File Name: input.c

	> Author: zhanglp

	> Mail: zhanglp92@gmail.com

	> Created Time: 2014年11月15日 星期六 19时45分54秒

	> Description: 
 ************************************************************************/

#if defined(_UCC)
    #include <stdio.h>
    #include <stdlib.h>
    #include <unistd.h>
#elif defined(_WIN32)
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
    #include <fcntl.h>
#endif

#include "input.h"
#include "error.h"

unsigned char END_OF_FILE = 255;
/* 保存输入文件的信息 */
struct input Input;
/* 在遇到＃include 的时候使用
 * Input 表示当前处理文件
 * PreInput 表示暂且挂起的文件 */
struct input PreInput;

/* 读取源文件内容并,并映射到对应的某一段内存上 */
void ReadSourceFile (char *filename) 
{
#if defined(_UCC) 

    /* 以只读的方式打开文件 */
    Input.file = fopen (filename, "r");
    if (NULL == Input.file) {

        Fatal ("Can't open file: %s.", filename);
    }
    /* 先定位到文件尾,读取文件的内容长度 
     * 并给文件申请同样大小的空间 */
    fseek (Input.file, 0, SEEK_END);
    Input.size = ftell (Input.file);
    Input.base = malloc (Input.size + 1);
    if (NULL == Input.base) {
        
        Fatal ("The file %s is too big", filename);
        fclose (Input.file);
    }
    /* 重新定位文件指针,并读取文件的所有内容 */
    fseek (Input.file, 0, SEEK_SET);
    Input.size = fread (Input.base, 1, Input.size, Input.file);
    fclose (Input.file);

#elif defined(_WIN32)

    Input.file = CreateFileA (filename, GENERIC_READ | GENERIC_WRITE, 
                0, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (INVALID_HANDLE_VALUE == Input.file) {
    
        Fatal ("Can't open file: %s.", filename);
    }
    Input.size = GetFileSize (Input.file, NULL);
    Input.fileMapping = CreateFileMapping (Input.file, NULL, 
                        PAGE_READWRITE, 0, Input.size + 1, NULL);
    if (NULL == Input.fileMapping) {
        
        Fatal ("Can't create file mapping: %s.", filename);
    }
    Input.base = (unsigned char*)MapViewOfFile (Input.fileMapping, 
                                FILE_MAP_WRITE, 0, 0, 0);
    if (NULL == Input.base) {
        
        Fatal ("Can't map file: %s.", filename);
    }
#else 

    struct stat st;
    long fno;

    fno = open (filename, O_RDWR);
    if (-1 == fno) {
    
        Fatal ("Can't open file %s.\n", filename);
    }
    if (-1 == fstat (fno, &st)) {
        
        Fatal ("Can't stat file %s. \n", filename);
    }
    Input.size = st.st_size;
    Input.base = mmap (NULL, Input.size + 1, PROT_WRITE,
                    MAP_PRIVATE, fno, 0);
    if (MAP_FAILED == Input.base) {
        
        Fatal ("Can't mmap file %s. \n", filename);
    }
    Input.file = (void*)fno;
#endif
    
    Input.filename = filename;
    /* 在文件末尾存入结束符号 */
    Input.base[Input.size] = END_OF_FILE;
    Input.current = Input.lineHead = Input.base;
    Input.line = 1;
}

/* 判断文件是否存在 */
int FileExist (const char *filename) 
{
#if defined(_UCC)
    return (access (filename, 0) == 0);
#else
    long fno;
    if ((fno = open (filename, O_RDWR)) == -1) {
        
        return 0;
    } else {

        close (fno);
        return 1;
    }
#endif
}

/* 关闭源文件并释放对应的内存空间 */
void CloseSourceFile (void) 
{
#if defined(_UCC) 
    free (Input.base);
#elif defined(_WIN32)
    UnmapViewOfFile (Input.base);
    CloseHandle (Input.fileMapping);
    SetFilePointer (Input.file, Input.size, NULL, FILE_BEGIN);
    CloseHandle (Input.file);
#else
    close ((long)Input.file);
    munmap (Input.base, Input.size + 1);
#endif
}
