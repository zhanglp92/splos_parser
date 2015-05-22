/*
 * =====================================================================================
 *
 *       Filename:  ctrl_directory.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/17/2015 08:00:50 PM
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
#include <string.h>
#include "mksfs.h"


int ctrl_directory_ls(struct sfs_mem_ctrl *ctrl)
{
	printf("\n--------ls-----------\n");
	unsigned int ix = 0;
	for(ix = 0; ix < SFS_BLOCK_SIZE / sizeof(struct dir_entry); ix++){
		if((ctrl->root)[ix].inode_number != -1){
			printf("%s : %d\n", (ctrl->root)[ix].dir_name, (ctrl->root)[ix].inode_number);
		}
	}
	return 0;
}

