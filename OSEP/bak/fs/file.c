/*
 * =====================================================================================
 *
 *       Filename:  file.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  04/01/2015 06:35:34 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "error.h"
#include "file.h"
#include "mksfs.h"


int strip_path(char *filename, char *pathname)
{
	
	if (*pathname == '/'){
		pathname++;
	}else{
		bug("the url is wrong\n");
		return -1;
	}
	while (*pathname != '\0'){

		if (*pathname == '/'){
			pathname ++;
			while (*pathname != '\0'){
				*filename = *pathname;
				filename++;
				pathname++;
			}
		}else{
			pathname++;
		}
	}
	*filename = '\0';
	return 0;
}

int search_file(struct sfs_mem_ctrl *ctrl, char *pathname)
{
    int value = -1;
	char *filename = (char *)malloc(SFS_MAX_FNAME_LEN + 1);
	assert(filename != NULL);
	
	strip_path(filename, pathname);
    
    unsigned int ix = 0;
	
	for (ix = 0; ix < SFS_BLOCK_SIZE / sizeof(struct dir_entry); ix ++){
		if (strcmp(filename, (ctrl->root)[ix].dir_name) == 0){
			value = (ctrl->root)[ix].inode_number;
			break;
		}
	}
	free(filename);
    return value;
}
