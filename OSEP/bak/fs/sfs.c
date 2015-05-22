/*
 * =====================================================================================
 *
 *       Filename:  sfs.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/06/2015 07:26:23 PM
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
#include "dev.h"
#include "sfs.h"
#include "mksfs.h"

struct sfs_mem_ctrl *Ctrl;

struct sfs_mem_ctrl *sfs_init()
{
	int value = ide_init_block("/tmp/file.txt");
    assert(value == 0);

	struct sfs_mem_ctrl *ctrl = (struct sfs_mem_ctrl*)malloc(sizeof(struct sfs_mem_ctrl));
    assert(ctrl != NULL);
	ctrl->superblock = (struct sfs_superblock *)malloc(SFS_BLOCK_SIZE);
	assert(ctrl->superblock != NULL);

    value = ide_read_block(ctrl, 0, (char*)ctrl->superblock, SFS_BLOCK_SIZE, 0);
    assert(value != 0);
    
    ctrl->blockmap = (struct sfs_blockbitmap *)malloc(sizeof(struct sfs_blockbitmap));
	assert(ctrl->blockmap != NULL);
	ctrl->blockmap->nbits = 32768;
	ctrl->blockmap->nwords = 1024;

	ctrl->blockmap->map = (unsigned int *)malloc(sizeof(unsigned int) * (ctrl->blockmap->nwords));
	assert(ctrl->blockmap->map != NULL);
	value = ide_read_block(ctrl, 1, (unsigned int *)ctrl->blockmap->map, sizeof(unsigned int) * (ctrl->blockmap->nwords), 0);

	ctrl->inodemap = (char *)malloc(SFS_BLOCK_SIZE);
	assert(ctrl->inodemap != NULL);
	value = ide_read_block(ctrl, 2, (char*)ctrl->inodemap, SFS_BLOCK_SIZE, 0);
	assert(value != 0);
  
	
	ctrl->inodetable = (struct sfs_inode *)malloc(sizeof(struct sfs_inode) * 4);
	assert(ctrl->inodetable != NULL);
    value = ide_read_block(ctrl, 3, (char*)ctrl->inodetable, sizeof(struct sfs_inode) * 4, 0);
	assert(value != 0);


	ctrl->root = (struct dir_entry*)malloc(SFS_BLOCK_SIZE);
	assert(ctrl->root != NULL);
	value = ide_read_block(ctrl, 35, (char*)ctrl->root, SFS_BLOCK_SIZE, 0);
	assert(value != 0);
    
	ctrl->inodetablelength = 4;
	return ctrl;
}

int return_sfs_mem_ctrl()
{
	Ctrl = sfs_init();
	printf("%d\n", Ctrl->blockmap->nwords);
	return 0;
}

