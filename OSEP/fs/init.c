/*
 * =====================================================================================
 *
 *       Filename:  test.h
 *
 *    Description:  测试用例的代码
 *
 *        Version:  1.0
 *        Created:  05/16/2015 08:15:57 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */
#include <string.h>
#include <sys/types.h>
#include <dirent.h>

#include "error.h"
#include "mksfs.h"
#include "fileinterface.h"
//初始化文件布局代码
int init_file_layout()
{
	DIR    *dir;
	struct  dirent *ptr;
	
	if ((dir = opendir("/tmp/")) == NULL){
		bug("opendir is error\n");
		return -1;
	}
	while((ptr = readdir(dir)) != NULL){
		if(strcmp(ptr->d_name, "file.txt") == 0){
			printf("磁盘初始化已经OK\n");
		    return 0;
		}
	}
	closedir(dir);
	printf("\n-----初始化磁盘-----------\n");
	struct sfs_mem_ctrl *ctrl;
	ctrl = sfs_create();
	write_info(ctrl);
	printf("\n-----磁盘初始化结束-------\n");

	return 0;
}
int init_memory_layout()
{
	return_sfs_mem_ctrl();
	test(Ctrl, 4);
    linit();
}

//测试ctrl_open(), ctrl_read(), ctrl_write(), ctrl_close()
int base_funcation_create(char *url, char *buf, int size)
{
	struct file_desc *ops = ctrl_open(url, LF_RDWR|LF_CREAT);
	ctrl_write(ops, buf, size);
	char buffer[50];
	memset(buffer, '\0', 50);
	ctrl_read(ops, buffer, size);
	printf("从文件读取的数据为:%s\n", buffer);
	ctrl_close(ops);
	return 0;
}

int base_funcation(char *url, int size)
{
	struct file_desc *ops = ctrl_open(url, LF_RDWR);
	char buffer[50];
	memset(buffer, '\0', 50);
	ctrl_read(ops, buffer, size);
	printf("从文件读取的数据为:%s\n", buffer);
	ctrl_close(ops);
}


