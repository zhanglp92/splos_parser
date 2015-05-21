/*
 * =====================================================================================
 *
 *       Filename:  sfs_inode.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/30/2015 03:08:08 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will:, there is a way
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "sfs_inode.h"
#include "mksfs.h"
#include "error.h"
#include "dev.h"

char *inodemap_create(unsigned int uinodes)
{
	assert(uinodes == SFS_INODE_SIZE);

	char *inodemap = (char *)malloc(sizeof(char) * SFS_INODE_SIZE);
	assert(inodemap != NULL);

	//初始化所有的inodemap;
    memset(inodemap, 1, SFS_INODE_SIZE);
	memset(inodemap, 0, 4);
	return inodemap;
}

struct sfs_inode * inodetable_create(unsigned int uinodes)
{

	assert(uinodes  == SFS_INODE_SIZE);
	struct sfs_inode *inodetable = (struct sfs_inode *)malloc(sizeof(struct sfs_inode) * SFS_INODE_SIZE);
	assert(inodetable != NULL);

	int ix = 0;

	for (ix = 0; ix < SFS_INODE_SIZE; ix++){
		inodetable[ix].i_inode = -1;
	}

	struct sfs_inode  root;
	root.i_inode = 0;
	root.i_type = SFS_TYPE_DIR;
	root.i_nlinks =0;
    root.i_size = sizeof(struct dir_entry) * 4;
	root.i_indirect = -1;
	root.i_direct[0] = 35;
	root.i_blocks = 1;
    inodetable[0] = root;

	struct sfs_inode pe;
	
	for (ix  =1 ; ix < 4; ix ++){
		pe.i_inode = ix;
		pe.i_type = SFS_TYPE_CHAR;
		pe.i_nlinks = 0;
		pe.i_size = 0;
        pe.i_blocks = 0;
		inodetable[ix] = pe;
	}

    return inodetable;
}

int inodemap_index(char *inodemap)
{
	int ix = 0;
	int index = -1;
	for (ix = 0; ix < SFS_INODE_SIZE; ix++){
		if (inodemap[ix] == 1){
			index  = ix;
			inodemap[ix] = 0;
			Ctrl->superblock->uinodes--;
	        ide_write_block(Ctrl, 0, Ctrl->superblock, SFS_BLOCK_SIZE, 0);
	        ide_write_block(Ctrl, 2, Ctrl->inodemap, SFS_BLOCK_SIZE, 0);
			return index;
		}
	}
	return index;
}

int inodemap_test(char *inodemap)
{
	unsigned int ix = 0;
	unsigned int count = 0;
	for (ix = 0; ix < SFS_INODE_SIZE; ix++){
		if (inodemap[ix] == 0){
            count++;
		}
	}
	printf("inode map %d\n", count);
	return 0;
}

int inodetable_test(struct sfs_inode *inodetable, int length)
{
	unsigned int ix = 0;
	unsigned int count = 0;
	for (ix = 0; ix < length; ix++){
		if (inodetable[ix].i_inode != -1){
			count ++;
		}
	}
	printf("inodetable %d\n", length);
	return 0;
}

struct sfs_inode *get_inode(struct sfs_mem_ctrl *ctrl,  int inode_num)
{
	unsigned int ix = 0;
	assert(inode_num < SFS_INODE_SIZE);

	struct sfs_inode *temp = ctrl->inodetable;
	for (ix = 0; ix < ctrl->inodetablelength; ix ++){
	    if (temp[ix].i_nlinks){
			if (temp[ix].i_inode == inode_num){
				temp[ix].i_nlinks++;
				return &temp[ix];
			}
		}
	}
	//读磁盘上的数据
    struct sfs_inode *inode = (struct sfs_inode*)malloc(sizeof(struct sfs_inode));
	int offset = inode_num * 128;
	int ret = ide_read_block(ctrl, 3, (char*)inode, 128, offset);
	if (ret == -1){
		bug("ide_read_block is error\n");
		return NULL;
	}
	ctrl->inodetablelength ++;
	inode->i_inode = inode_num;
	inode->i_nlinks = 1;  
    //ctrl->inodetable = realloc(ctrl->inodetable,  ctrl->inodetablelength);
//	ctrl->inodetable[ctrl->inodetablelength - 1] = inode;
    return inode;  
//	return &(ctrl->inodetable[ctrl->inodetablelength - 1]);
}

struct sfs_inode *new_inode(struct sfs_mem_ctrl *ctrl, int inode_num)
{
	struct sfs_inode *new_inode = get_inode(ctrl, inode_num);
	new_inode->i_blocks = 0;
	new_inode->i_indirect = -1;
	new_inode->i_size = 0;
	new_inode->i_type = SFS_TYPE_FILE;
	sync_inode(ctrl, new_inode);
	printf("the new inode size %d\n", new_inode->i_size);
	return new_inode;
}

//保持磁盘和缓冲区的一致
int sync_inode(struct sfs_mem_ctrl *ctrl,  struct sfs_inode *inode)
{
	int offset = (inode->i_inode) * sizeof(struct sfs_inode);
	int ret = ide_write_block(ctrl, 3, (char*)inode, 128, offset);
    if(ret == -1){
		bug("ide_write_block is error");
		return -1;
	}
	return 0;
}

//释放inode
void free_inode(struct sfs_inode *inode)
{
	assert(inode->i_nlinks > 0);
	inode->i_nlinks --;
}

int new_dir_entry(struct sfs_mem_ctrl *ctrl, int inode_num, char *filename)
{
	unsigned int ix = 0;
	for(ix = 0; ix < SFS_BLOCK_SIZE / (sizeof(struct dir_entry)); ix ++){
		if((ctrl->root)[ix].inode_number == -1){
            (ctrl->root)[ix].inode_number = inode_num;
			strcpy((ctrl->root)[ix].dir_name, filename);
            (ctrl->inodetable)[0].i_size++;
			break;
		}	    
	}
    ide_write_block(ctrl, 35,  (char*)ctrl->root, SFS_BLOCK_SIZE, 0);
    ide_write_block(ctrl, 3, (char*)ctrl->inodetable, sizeof(struct sfs_inode), 0);
	return 0;
}

int free_inodemap(struct sfs_mem_ctrl *ctrl, int index)
{
	ctrl->inodemap[index] = 1;
	ide_write_block(ctrl, 2, (char*)ctrl->inodemap, SFS_BLOCK_SIZE, 0);
	return 0;
}

int free_inodetable(struct sfs_inode *inode, int index)
{
	int ix = 0;
    for(ix = 0; ix < 12; ix++){
		inode->i_direct[ix] = 0;
	}
	int offset = index * sizeof(struct sfs_inode);
	ide_write_block(Ctrl, 3, (char*)inode, sizeof(struct sfs_inode), offset);
	
	return 0;
}

int free_dentry(struct sfs_mem_ctrl *ctrl, int inode_num)
{
	unsigned int ix = 0;
	for(ix = 0; ix < SFS_BLOCK_SIZE / (sizeof(struct dir_entry)); ix ++){
		if((ctrl->root)[ix].inode_number == inode_num){
            (ctrl->root)[ix].inode_number = -1;
		    (ctrl->inodetable)[0].i_size--;
			break;
		}	    
	}
    ide_write_block(ctrl, 35,  (char*)ctrl->root, SFS_BLOCK_SIZE, 0);
    ide_write_block(ctrl, 3, (char*)ctrl->inodetable, sizeof(struct sfs_inode), 0);
	return 0;
}

