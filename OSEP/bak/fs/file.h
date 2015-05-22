/*
 * =====================================================================================
 *
 *       Filename:  file.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/01/2015 02:38:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */

#ifndef __FILE_H__
#define __FILE_H__

#include "mksfs.h"

#define LF_CREAT  0x0001
#define LF_RDWR   0x0010

struct file_desc{
	int      fd_mode;   //R|W
	int      fd_pos;       //当前读写的位置
	struct   sfs_inode *inode;
};
//这个值应该是进程中的
//extern struct file_desc *file[1024];
//extern struct file_desc *file_desc_table[1024];

int strip_path(char *filename, char *pathname);
int search_file(struct sfs_mem_ctrl *ctrl, char *pathname);

struct file_desc *ctrl_open(char *url, int flag);
int ctrl_write(struct file_desc *desc, char *buf, int size);
int ctrl_read(struct file_desc *desc, char *buf, int size);
int ctrl_close(struct file_desc *desc);
int ctrl_lseek(struct file_desc *desc, int pos);
int ctrl_delete(char *path);
int ctrl_directory_ls(struct sfs_mem_ctrl *ctrl);

#endif //__FILE_H__
