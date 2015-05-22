/*
 * =====================================================================================
 *
 *       Filename:  fileinterface.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/19/2015 02:22:22 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fileinterface.h"
#include "dev.h"

int linit()
{
	unsigned int ix = 0;
	for (ix = 3; ix < 1024; ix ++){
        file[ix] = NULL;
	}
    return 0;
}

int lfopen(char *pathname, int flag)
{
    unsigned int ix = 0;
	for (ix = 3; ix < 1024; ix++){
         if (file[ix] == NULL){
			 break;
		 }
	}
	struct file_desc *ops = ctrl_open(pathname, flag);
	if (ops == NULL){
		return -1;
	}
    file[ix] = ops;
	return ix;
}

int lfseek(int fd, int offset)
{
	struct file_desc *ops = file[fd];
    ctrl_lseek(ops, offset);
	return 0;
}
int lfread(int fd, char *buffersize, int length)
{
	struct file_desc *ops = file[fd];
	int count;
	count = ctrl_read(ops, buffersize, length);
	return count;
}

int lfwrite(int fd, char *buffersize, int length)
{
	struct file_desc *ops = file[fd];
	int count;
    count = ctrl_write(ops, buffersize, length);
	return count;
}

int lfclose(int fd)
{
	struct file_desc *ops = file[fd];
    ctrl_close(ops);
	file[fd] = NULL;
	return 0;
}

int laccess(char *pathname, int flag)
{
	int value = search_file(Ctrl, pathname);
	if(value == -1){
		printf("the file is not find\n");
		return value;
	}else {
		return value;
	}
}

int lfstat(int fd, int *index)
{
    struct file_desc *ops = file[fd];
	struct sfs_inode  inode;
	ide_read_block(Ctrl, 3, (char *)&inode, sizeof(struct sfs_inode), ops->inode->i_inode * sizeof(struct sfs_inode));
	*index = inode.i_size;
    printf ("size: %d \n", *index);
	return 0;
}
int copyfile(char *pathname, char *filename)
{
	int op = open(filename, O_RDWR);
	int lp = lfopen(pathname, LF_RDWR|LF_CREAT);
	int ret = 0;
	char buf[4096];
	memset(buf, '\0', 4096);
	while ( (ret = read(op, buf, 4095)) >0){
		lfwrite(lp, buf, ret);
		memset(buf, '\0', 4096);
	}
	close(op);
	lfclose(lp);
	return 0;
}

