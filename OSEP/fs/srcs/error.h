/*
 * =====================================================================================
 *
 *       Filename:  error.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/29/2015 09:24:40 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */
#ifndef __ERROR_H__
#define __ERROR_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define __error(msg, quit, ...)                                                    \
	do {                                                                           \
		fprintf(stderr, #msg ": function %s - line %d: " , __FUNCTION__, __LINE__);\
		if (errno != 0 ){                                                          \
			fprintf( stderr, "[error] %s :", strerror(errno));                     \
		}                                                                          \
		fprintf(stderr, "\n\t");                                                   \
		fprintf(stderr, __VA_ARGS__);                                              \
		errno = 0;                                                                 \
		if (quit){                                                                 \
			exit(-1);                                                              \
		}                                                                          \
    }while(0);                                                                     

#define warn(...)      __error(warn, 0,  __VA_ARGS__)
#define bug(...)       __error(bug,  1,  __VA_ARGS__)
#define info(...)      __error(info, 0,  __VA_ARGS__)

#endif //__ERROR_H__
