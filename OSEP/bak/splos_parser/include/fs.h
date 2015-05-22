/*************************************************************************
	> File Name: test_fileinterface.h
	> Author: 
	> Mail: 
	> Created Time: 2015年05月21日 星期四 17时28分27秒
 ************************************************************************/

#ifndef _TEST_FILEINTERFACE_H
#define _TEST_FILEINTERFACE_H


#include "fs/file.h"
#include "fs/fileinterface.h"


#define LF_FILE \
    char lf_file_[SFS_MAX_FNAME_LEN] = {}; \
    sprintf (lf_file_, "/root/%s", pathname)


int LfOpen (char *pathname, int flag)
{
    LF_FILE;
    return lfopen (lf_file_, flag);
}

int LfAccess(char *pathname, int flag) 
{
    LF_FILE;
    return laccess (lf_file_, flag);
}

int LfCopyfile(char *pathname, char *filename)
{
    LF_FILE;
    return copyfile (lf_file_, filename);
}

#endif
