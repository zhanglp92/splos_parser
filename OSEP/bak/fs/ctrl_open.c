/*
 * =====================================================================================
 *
 *       Filename:  ctrl_open.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/01/2015 06:38:49 PM
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


#include "sfs_inode.h"
#include "blockbitmap.h"
#include "file.h"
#include "error.h"
#include "mksfs.h"

//struct file_desc *file[1024];
//struct file_desc *file_desc_table[1024];

//返回值: file[1024]中的一个索引
/* 
 * (1)在file寻找一个空项
 *（2)在file_desc_table[1024]中寻找一个空项
   (3)检测文件是否存在，不存在则需要做几下几件事情
      1>为文件内容分配扇区
 */

static struct sfs_inode *ctrl_create(char *url, int flag);

struct file_desc *ctrl_open(char *url, int flags)
{
//	unsigned int  ix = 0;
    struct file_desc  *desc = (struct file_desc *)malloc(sizeof(struct file_desc));
	assert(desc != NULL);
	int inode_num = search_file(Ctrl, url);
    
	struct sfs_inode  *inode = NULL;
    if (flags & LF_CREAT){
		if (inode_num != -1){
			bug("the file is exists\n");
			return NULL;
		}else if(inode_num == -1){
			inode = ctrl_create(url, flags);
		}
	}else{
		if (inode_num == -1){
			printf("the file is not exist\n");
			return NULL;
		}
		inode = get_inode(Ctrl, inode_num);
	}
	if (inode){
		desc->fd_mode = LF_RDWR;
		desc->fd_pos = 0;
		desc->inode = inode;
//		file[fd] = &file_desc_table[ix];
//		file_desc_table[ix]->fd_mode = flags;
//		file_desc_table[ix]->fd_pos = 0;
//		file_desc_table[ix]->inode = inode;
	}
	return desc;
}

/*
 * inode map 中找到一个合适的位置
 * 在blockmap中找到一个合适的位置
 * 在indoe table 中分配一个合适的位置
 * 在目录项中增加一项
 */
static struct sfs_inode *ctrl_create(char *url, int flag)
{

	char filename[SFS_MAX_FNAME_LEN + 1];
    int  value = strip_path(filename, url);
    assert(value == 0);

    int inode_num = inodemap_index(Ctrl->inodemap);
    printf("the inode_num is [%d]\n", inode_num);  

//    int ret = blockbitmap_alloc(Ctrl->blockmap, &index_store);  
//	assert(ret != -1);
    
	struct sfs_inode  *inode = new_inode(Ctrl, inode_num);
	new_dir_entry(Ctrl, inode_num, filename);

	return inode;
}

