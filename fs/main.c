/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  05/14/2015 10:17:37 AM
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
#include "blockbitmap.h"
#include "mksfs.h"
#include "fileinterface.h"


int main(int argc, char *argv[])
{
    struct sfs_mem_ctrl *ctrl;
 	ctrl = sfs_create();
    write_info(ctrl);
	
   
/*  	printf("\n----------------------------\n");
    return_sfs_mem_ctrl();
	test(Ctrl, 4);
  */
//	init_file_layout();
	init_memory_layout();

	int size;
	 copyfile("/root/1.txt",  "1.txt");
    int lt = lfopen("/root/1.txt" , LF_RDWR);
    lfstat("/root/1.txt", &size);
    char buf[size + 1];
	memset(buf, '\0', size + 1);
    lfread(lt, buf, size);
	printf("%s\n", buf);
	
//    struct file_desc *opss = ctrl_open("/root/1.txt", O_RDWR);
//    ctrl_write(opss, "helloworld", 12);
//	test(Ctrl, Ctrl->inodetablelength);
//	memset(buf, '\0',50);
//	ctrl_read(opss, buf, 12);
//	printf("%s\n", buf);

   	//	test(Ctrl, Ctrl->inodetablelength);
	return EXIT_SUCCESS;
}


