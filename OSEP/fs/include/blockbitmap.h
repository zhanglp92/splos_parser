/*
 * =====================================================================================
 *
 *       Filename:  blockbitmap.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/30/2015 02:50:04 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */
#ifndef __BLOCKBITMAP_H__
#define __BLOCKBITMAP_H__

#include "mksfs.h"
struct sfs_blockbitmap* blockbitmap_create(unsigned int nbits); 

int blockbitmap_alloc(struct sfs_blockbitmap *bitmap, unsigned int *index_store);
int blockbitmap_translate(struct sfs_blockbitmap *bitmap, unsigned int index, 
		                   unsigned int **word,  unsigned  int *mask);
void blockbitmap_free(struct sfs_blockbitmap *bitmap, unsigned int index);
void blockbitmap_destory(struct sfs_blockbitmap *bitmap);
int blockbitmap_test(struct sfs_blockbitmap *bitmap);

#endif //__BLOCKBITMAP_H__

