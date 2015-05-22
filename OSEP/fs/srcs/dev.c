/*
 * =====================================================================================
 *
 *       Filename:  dev.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/01/2015 04:41:42 PM
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
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mksfs.h"
#include "dev.h"
#include "error.h"

int fd;
int ide_init_block(char *url)
{
	fd = open(url, O_CREAT|O_RDWR, 0666);
	if (fd == -1){
		bug("open file is error");
		return -1;
	}
	return 0;
}

int ide_write_block(struct sfs_mem_ctrl  *ctrl, int bnum,  char *buffer,  int buffersize, int offset)
{
//	assert(bnum < ctrl->superblock->blocks);
    int count;
	int ret = lseek(fd, (bnum * SFS_BLOCK_SIZE) + offset, SEEK_SET);
	if (ret == -1){
		bug("ide_write_block lseek is error");
		return -1;
	}
	do{
		count = write(fd, buffer, buffersize);
		if (count == -1){
			bug("write is failed\n");
			return -1;
		}else if (errno == EAGAIN){
			continue;
		}else if(count == buffersize){
			break;
		}
	}while(1);
	return count;
}

int ide_read_block( struct sfs_mem_ctrl *ctrl, int bnum, char *buffer, int buffersize, int offset)
{
//	assert(bnum < ctrl->superblock->blocks);
    int count;
	int ret = lseek(fd, (bnum * SFS_BLOCK_SIZE) +offset, SEEK_SET);
	if(ret == -1){
		bug("ide_read_block lseek is error\n");
		return -1;
	}
	do{
		count = read(fd, buffer, buffersize);
		if (count == -1){
			bug("read is error");
			return -1;
		}else if(count == EAGAIN){
			continue;
		}else {
			break;
		}
	}while(1);
	printf("%d\n",count);
	return count;
}


