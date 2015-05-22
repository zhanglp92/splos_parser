/*
 * =====================================================================================
 *
 *       Filename:  ctrl_delect.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/15/2015 04:28:44 PM
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

#include "mksfs.h"
#include "file.h"
#include "dev.h"
#include "blockbitmap.h"
#include "sfs_inode.h"
//释放inode_map中的相应位
//释放blockmap中相应位
//将inode_array中的inode位设置为空
//删除根目录中的目录项

int ctrl_delete(char *pathname)
{
	int inode_num = search_file(Ctrl, pathname);
	if (inode_num == -1){
		printf("the file is not find \n");
        return -1;
	}
	struct sfs_inode *inode = (struct sfs_inode *)malloc(sizeof(struct sfs_inode));
    assert(inode != NULL);

	ide_read_block(Ctrl, 3, (char *)inode, sizeof(struct sfs_inode), inode_num * sizeof(struct sfs_inode));
    free_inodemap(Ctrl, inode_num);

	int ix;
	for(ix = 0; ix < 12; ix ++){
		if(inode->i_direct[ix]!= 0){
			
			blockbitmap_free(Ctrl->blockmap, inode->i_direct[ix]);
		}
	}
	free_inodetable(inode, inode_num);
	free_dentry(Ctrl, inode_num);
	free(inode);
	return 0;
}

