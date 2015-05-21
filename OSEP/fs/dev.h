/*
 * =====================================================================================
 *
 *       Filename:  dev.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/01/2015 04:33:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */

#ifndef __DEV_H__
#define __DEV_H__

#include "mksfs.h"
int ide_init_block(char *url);

int ide_read_block(struct sfs_mem_ctrl *ctrl, int bnum,  char *buffer, int buffersize,  int offset);

int ide_write_block(struct sfs_mem_ctrl *ctrl, int bnum, char *buffer, int buffersize,  int offset);

extern int fd;

#endif //__DEV_H__


