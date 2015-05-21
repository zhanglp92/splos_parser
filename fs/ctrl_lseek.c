/*
 * =====================================================================================
 *
 *       Filename:  ctrl_lseek.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/15/2015 04:25:29 PM
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

int ctrl_lseek(struct file_desc *desc, int pos)
{
	desc->fd_pos = pos;
	return pos;
}

