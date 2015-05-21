/*
 * =====================================================================================
 *
 *       Filename:  ctrl_write.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/10/2015 08:21:44 PM
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
#include "mksfs.h"
#include "file.h"
#include "sfs_inode.h"
#include "blockbitmap.h"

/*
int ctrl_write(struct file_desc *desc, char *buf, int size)
{

	struct sfs_inode *inode = desc->inode;
	unsigned int ix = 0;
    int *database;
	unsigned int baddr = desc->fd_pos;
	unsigned int bondary = baddr % SFS_BLOCK_SIZE;
	unsigned int max_block = (baddr + size) / SFS_BLOCK_SIZE;
	unsigned int curr_block = (inode->i_size) / SFS_BLOCK_SIZE ;
	//第一次写
	if (inode->i_size == 0){
		//直接写入第一次分配好的块中（由于之前在创建文件的时候已经分配块了）

	}
    if (max_block > curr_block && max_block <= 12){
		ix  = curr_block + 1;
		for (; ix < max_block; ix++){
			int nblock;
			int ret = blockbitmap_alloc(Ctrl->blockmap, &nblock);
			assert(ret != -1);
			printf("Nblock is %d\n", nblock);
			inode->i_direct[ix] = nblock;
			inode->i_blocks ++;
		}
		inode->i_indirect = -1;

	}else if (max_block > 12 & max_block > curr_block){
        ix = curr_block + 1;
		for (; ix < max_block - 12; ix ++){
			int nblock; 
			int ret = blockbitmap_alloc(Ctrl->inodemap, &nblock);
			assert(ret != -1);
			inode->i_direct[ix] = nblock;
			inode->i_blocks++;
		}
		if (inode->i_indirect == -1){
			int nblock;
			int ret = blockbitmap_alloc(Ctrl->inodemap, &nblock);
			assert(ret != -1);
			inode->i_indirect = nblock;
            database = (int *)malloc(SFS_BLOCK_SIZE);
			assert(database != NULL);
			for (ix = 0; ix < SFS_BLOCK_SIZE / sizeof(int); ix ++){
				database[ix] = -1;
			}
			for (ix = 12; ix < max_block; ix ++){
				ret = blockbitmap_alloc(Ctrl->blockmap, &nblock);
				assert(ret != -1);
				database[ix - 12] = nblock;
			}
			int offset = (inode->i_indirect) * SFS_BLOCK_SIZE;
		    ret = ide_write_block(Ctrl, inode->i_indirect, (char *)database, SFS_BLOCK_SIZE, offset);
		    assert(ret != -1);	
		}else {
            database = (int *)malloc(SFS_BLOCK_SiZE);
			assert(database != NULL);
			int offset = (inode->i_indirect) * SFS_BLOCK_SIZE;
			ret = ide_read_block(Ctrl, inode->i_indirect, (char *)database, SFS_BLOCK_SIZE, offset);
			assert(ret != -1);
			int j = 0;
			for (ix = 12; ix < max_block; ix++){
				if (database[j] == -1){	
					int nblock;
			        int ret = blockbitmap_alloc(Ctrl->inodemap, &nblock);
			        assert(ret != -1);
					database[ix - 12] = nblock;
				}else if (j < SFS_BLOCK_SIZE / sizeof(int)){
					j++;
				}else {
                    printf("the file is too big");
					return -1;
				}
			}
			offset = (inode->i_indirect) * SFS_BLOCK_SIZE;
			ret = ide_write_block(Ctrl, inode->i_indirect, (char *)database, SFS_BLOCK_SIZE, offset);
			assert(ret != -1);

		}

	}
	int count = 0;
	for (ix = baddr / SFS_BLCOK_SIZE; ix < max_block; ix ++){
		if(inode->i_indirect == -1){
			int offset = (inode->i_direct[ix]) * SFS_BLOCK_SIZE + bondary;
			count += ide_write_block(Ctrl, inode->i_direct[ix], buf + size, (size - count)% SFS_BLOCK_SIZE, offset);
			bondary = count % SFS_BLOCK_SIZE; 
		} else if ( inode->i_indirect != -1 || ix >= 12){
			if (ix < 12){
				int offset = (inode->i_direct[ix]) * SFS_BLOCK_SIZE + bondary;
		 	    count += ide_write_block(Ctrl, inode->i_direct[ix], buf + size, (size - count)% SFS_BLOCK_SIZE, offset);
			    bondary = count % SFS_BLOCK_SIZE; 
		    }else {
				
			}
			
		}
		
	}
}
*/
int ctrl_write(struct file_desc *desc, char *buf, int size)
{
	//size = strlen(buf);
    struct sfs_inode *inode = desc->inode;
	unsigned int ix = 0;
 	unsigned int baddr = desc->fd_pos;
	unsigned int bondary = baddr % SFS_BLOCK_SIZE;
	unsigned int max_block = (baddr + size) / SFS_BLOCK_SIZE + 1;
	unsigned int curr_block = (inode->i_size) / SFS_BLOCK_SIZE ;
	unsigned int nblock;
	int ret;
	//第一次写
	if (inode->i_size == 0){
		ret = blockbitmap_alloc(Ctrl->blockmap, &nblock);
		printf("Nblock %d\n", nblock);
		inode->i_direct[0] = nblock;
		inode->i_blocks ++;

	}
    if (max_block > curr_block && max_block < 12){
		ix  = curr_block + 1;
		for (; ix < max_block; ix++){
			ret = blockbitmap_alloc(Ctrl->blockmap, &nblock);
			assert(ret != -1);
			printf("Nblock is %d\n", nblock);
			inode->i_direct[ix] = nblock;
			inode->i_blocks ++;
		}
	}
	int count = 0;
	int ux = 0;
	for (ix = baddr / SFS_BLOCK_SIZE; ix < max_block;){
		printf("the inode->direct[%d] %d\n", ix, inode->i_direct[ix]);
		if (size <= (SFS_BLOCK_SIZE - bondary)){
			count += ide_write_block(Ctrl, inode->i_direct[ix], buf,  size, bondary);
			break;
		}else if (size > (SFS_BLOCK_SIZE - bondary)){
			count += ide_write_block(Ctrl, inode->i_direct[ix], buf, SFS_BLOCK_SIZE - bondary, bondary);
			ix ++;
			for (ux = 0; ux < (size - count) / SFS_BLOCK_SIZE; ux++){
				count += ide_write_block(Ctrl, inode->i_direct[ix], buf + count, SFS_BLOCK_SIZE, 0);
				ix ++;
			}
			count += ide_write_block(Ctrl, inode->i_direct[ix], buf + count, (size - count)% SFS_BLOCK_SIZE, 0);
			ix ++;
		}
	}

	inode->i_size = inode->i_size + count;
	sync_inode(Ctrl, inode);
	desc->fd_pos = inode->i_size;
	return count;
}
