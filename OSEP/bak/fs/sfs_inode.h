/*
 * =====================================================================================
 *
 *       Filename:  sfs_inode.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/30/2015 03:04:51 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */

#ifndef __SFS_INODE_H__
#define __SFS_INODE_H__

#include "mksfs.h"
char * inodemap_create(unsigned int uinodes);
int inodemap_index(char *inodemap);

struct sfs_inode * inodetable_create(unsigned int uinodes);
struct sfs_inode *get_inode(struct sfs_mem_ctrl *ctrl, int inode_num);
struct sfs_inode *new_inode(struct sfs_mem_ctrl *ctrl, int inode_num);

int new_dir_entry(struct sfs_mem_ctrl *ctrl, int inode_num, char *filename);
int sync_inode(struct sfs_mem_ctrl *ctrl, struct sfs_inode *inode);
void free_inode(struct sfs_inode *inode);

int inodemap_test(char *inodemap);
int inodetable_test(struct sfs_inode *inodetable, int length);

int free_inodemap(struct sfs_mem_ctrl *ctrl, int index);
int free_inodetable(struct sfs_inode *inode, int index);
int free_dentry(struct sfs_mem_ctrl *ctrl, int inode_num);
#endif //__SFS_INODE_H__

