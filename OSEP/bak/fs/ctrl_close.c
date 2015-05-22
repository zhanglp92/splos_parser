/*
 * =====================================================================================
 *
 *       Filename:  ctrl_close.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/15/2015 04:19:30 PM
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

#include "file.h"
#include "sfs_inode.h"

int ctrl_close(struct file_desc *desc)
{
	
	free_inode(desc->inode);
	return 0;
}
