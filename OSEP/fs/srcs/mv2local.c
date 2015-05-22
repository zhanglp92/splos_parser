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
#include "init.h"
#include "fileinterface.h"
int main(int argc, char *argv[])
{
	
	init_file_layout();
	init_memory_layout();
//	copyfile("/root/file.p", "./1.txt");
	copyfiletolocal("/root/file.p", "./zero");
	return 0;
}


