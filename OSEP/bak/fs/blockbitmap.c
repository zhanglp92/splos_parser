/*
 * =====================================================================================
 *
 *       Filename:  blockbitmap.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/29/2015 10:10:20 PM
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
#include <string.h>
#include <assert.h>

#include "mksfs.h"
#include "error.h"
#include "dev.h"
#include "blockbitmap.h"

#define CHAR_BIT      (sizeof(char) * 8)
#define WORD_BIT      (sizeof(unsigned int) * CHAR_BIT)

struct sfs_blockbitmap *blockbitmap_create(unsigned int nbits)
{
	struct sfs_blockbitmap *bitmap =(struct sfs_blockbitmap*)malloc(sizeof(struct sfs_blockbitmap));
	assert(bitmap != NULL);
	
	bitmap->nbits = nbits;
	bitmap->nwords = nbits /(sizeof(unsigned int) * CHAR_BIT);
	bitmap->map = (unsigned int*)malloc(sizeof(unsigned int) * (bitmap->nwords));
	assert(bitmap->map != NULL);
    memset(bitmap->map, 0xff, sizeof(unsigned int) * bitmap->nwords);

	//将0～35 全部设置为0，表示已经使用过

	bitmap->map[0] = 0;
	bitmap->map[1] = 0xfffffff0;
	return bitmap;
}

int blockbitmap_alloc(struct sfs_blockbitmap* bitmap, unsigned int *index_store)
{
	unsigned int ix = 0;
	unsigned int offset;
	unsigned int mask;
   
    for (ix = 0; ix < bitmap->nwords; ix++){
		if (bitmap->map[ix] != 0){
			for (offset = 0 ; offset < WORD_BIT; offset++){
				mask = (1 << offset);
				if (bitmap->map[ix]  & mask){
					bitmap->map[ix] = bitmap->map[ix] ^ mask;
                    *index_store = ix * WORD_BIT + offset;
                    Ctrl->superblock->ublocks --;
	            	ide_write_block(Ctrl, 0, Ctrl->superblock, SFS_BLOCK_SIZE, 0);
	                ide_write_block(Ctrl, 1, Ctrl->blockmap->map, SFS_BLOCK_SIZE, 0);
					return 0;
				}
			}
		}
	}
	bug("there has no block");
	return -1;
}

int 
blockbitmap_translate(struct sfs_blockbitmap *bitmap, unsigned int index, 
		                          unsigned int **word, unsigned int *mask)
{
	assert(index < bitmap->nbits);
    unsigned int ix = index / WORD_BIT;
	unsigned int offset = index % WORD_BIT;
	*word =  bitmap->map + ix;
	*mask = (1 << offset);
	return 0;
}

void blockbitmap_free(struct sfs_blockbitmap *bitmap, unsigned int index)
{
	unsigned int *word, mask;
	blockbitmap_translate(bitmap, index, &word,  &mask);
    *word |= mask;
	ide_write_block(Ctrl, 1, (char*)bitmap->map, SFS_BLOCK_SIZE, 0);
}

void blockbitmap_destory(struct sfs_blockbitmap *bitmap)
{
	free(bitmap->map);
	free(bitmap);
}
int blockbitmap_test(struct sfs_blockbitmap *bitmap)
{
	unsigned int    ix =0;
	unsigned int    offset;
	unsigned int    mask;
	unsigned int    count = 0;

	for (ix = 0; ix < bitmap->nwords; ix ++){
        if(bitmap->map[ix] != 0xffffffff){
			for (offset = 0; offset < WORD_BIT; offset++){
                 mask = 1 << offset;
				 if (!(bitmap->map[ix] & mask)){
					 count ++;
				 }
			}
		}
	}
	printf("the block count is %d\n", count);
	return 0;
}

