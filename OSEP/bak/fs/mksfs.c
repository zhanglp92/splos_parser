/*
 * =====================================================================================
 *
 *       Filename:  mksfs.c
 *
 *    Description:  初始化文件系统
 *
 *        Version:  1.0
 *        Created:  03/29/2015 09:17:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */

#include <assert.h>
#include "error.h"
#include "mksfs.h"
#include "blockbitmap.h"
#include "sfs_inode.h"
#include "dev.h"

struct dir_entry * root_create()
{
	struct dir_entry *root = (struct dir_entry *)malloc (SFS_BLOCK_SIZE);
	assert(root !=  NULL);
    unsigned int ix = 0;
	for (ix = 0; ix < SFS_BLOCK_SIZE / (sizeof(struct dir_entry)); ix++){
		root[ix].inode_number = -1; 
	}
    strcpy(root[0].dir_name,  ".");
	root[0].inode_number = 0;
    
	
	for(ix = 1; ix < 4; ix++){
		sprintf(root[ix].dir_name, "dev_tty%d", ix - 1);
		root[ix].inode_number = ix ;
	}
	return root;
}

int root_test(struct dir_entry *root)
{
    unsigned int ix = 0;
	for (ix = 0; ix < SFS_BLOCK_SIZE / (sizeof(struct dir_entry)); ix++){
		if (root[ix].inode_number != -1){
			printf("dir_name: %s : inode: %d\n", root[ix].dir_name, root[ix].inode_number);
		}
    }
	return 0;
}
struct sfs_mem_ctrl * sfs_create()
{
	struct sfs_mem_ctrl *ctrl = (struct sfs_mem_ctrl *)malloc(sizeof(struct sfs_mem_ctrl));
	if (ctrl == NULL){
		bug("malloc sfs_mem_ctrl error");
		return NULL;
	}
	ctrl->superblock = (struct sfs_superblock *)malloc(sizeof(struct sfs_superblock));
	if (ctrl->superblock == NULL){
		free(ctrl);
		bug("malloc superblock error");
		return NULL;
	}
	ctrl->blockmap = (struct sfs_blockbitmap *)malloc (sizeof(struct sfs_blockbitmap));
	if (ctrl->blockmap == NULL){
		free(ctrl->superblock);
		free(ctrl);
		bug("malloc blockmap  error");
		return NULL;
	}
	
    ctrl->superblock->magic = SFS_MAGIC;
    ctrl->superblock->ublocks = (SFS_MAX_FILESYSTEM / SFS_BLOCK_SIZE) -  36;
	ctrl->superblock->blocks = (SFS_MAX_FILESYSTEM / SFS_BLOCK_SIZE);
	ctrl->superblock->inodes = SFS_INODE_SIZE;
    ctrl->superblock->uinodes = SFS_INODE_SIZE - 4;


	ctrl->blockmap = blockbitmap_create(SFS_MAX_FILESYSTEM / SFS_BLOCK_SIZE);
	ctrl->inodemap = inodemap_create(SFS_INODE_SIZE);
	ctrl->inodetable = inodetable_create(SFS_INODE_SIZE);
	ctrl->root = root_create();

	return ctrl;
}

int write_info(struct sfs_mem_ctrl *ctrl)
{
	int value = ide_init_block("/tmp/file.txt");
    assert(value == 0);
	
	//superblock to ide
	value = ide_write_block(ctrl, 0, ctrl->superblock, SFS_BLOCK_SIZE, 0);
	assert(value !=0);

	//blockbitmap to ide
	assert(value != 0);
	value = ide_write_block(ctrl, 1, ctrl->blockmap->map, sizeof(unsigned int)*(ctrl->blockmap->nwords), 0);

	//inodemap to ide
	value = ide_write_block(ctrl, 2, ctrl->inodemap, SFS_BLOCK_SIZE, 0);
    assert(value != 0);
    
	//inodetable to ide
	value = ide_write_block(ctrl, 3, ctrl->inodetable, SFS_BLOCK_SIZE * 32, 0);
	assert(value != 0);
   
	//root to ide
	value = ide_write_block(ctrl, 35, ctrl->root, SFS_BLOCK_SIZE, 0);
	assert(value != 0);
	
	free(ctrl->superblock);
	free(ctrl->blockmap->map);
	free(ctrl->blockmap);
	free(ctrl->inodemap);
	free(ctrl->inodetable);
	free(ctrl->root);
	return 0;
}

struct sfs_mem_ctrl * read_info()
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
    
	ctrl->inodetable = (struct sfs_inode *)malloc(SFS_BLOCK_SIZE * 32);
	assert(ctrl->inodetable != NULL);
	value = ide_read_block(ctrl, 3, (char*)ctrl->inodetable, SFS_BLOCK_SIZE * 32, 0);
	assert(value != 0);

	ctrl->root = (struct dir_entry*)malloc(SFS_BLOCK_SIZE);
	assert(ctrl->root != NULL);
	value = ide_read_block(ctrl, 35, (char*)ctrl->root, SFS_BLOCK_SIZE, 0);
	assert(value != 0);

	return ctrl;
}

int test(struct sfs_mem_ctrl *ctrl, int count)
{
	// Superblock:
	printf("MAGIC: %d\n", ctrl->superblock->magic);
	printf("BLOCKS: %d\n", ctrl->superblock->blocks);
	printf("UBLOCKS: %d\n", ctrl->superblock->ublocks);
	printf("INODES: %d\nn", ctrl->superblock->inodes);
	printf("UINODES:%d\n", ctrl->superblock->uinodes);

	//BLOCKBITMAP
    blockbitmap_test(ctrl->blockmap);
    //INODEMAP
    inodemap_test(ctrl->inodemap);
	//INODETABLE
    inodetable_test(ctrl->inodetable, count);
	//ROOT
    root_test(ctrl->root);
	return 0;
}

