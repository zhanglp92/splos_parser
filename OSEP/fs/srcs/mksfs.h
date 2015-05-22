/*
 * =====================================================================================
 *
 *       Filename:  mksfs.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  03/29/2015 08:31:29 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  kristychen (chenluwen), eallyclw@gmail.com
 *        Company:  where there is a will, there is a way
 *
 * =====================================================================================
 */
#ifndef __MKSFS_H__
#define __MKSFS_H__


#define   SFS_MAGIC            0x11
#define   SFS_INODE_SIZE       1024
#define   SFS_BLOCK_SIZE       4096
#define   SFS_MAX_FNAME_LEN    31
#define   SFS_MAX_FILESYSTEM   (1024UL * 1024 * 128)
#define   SFS_BLKN_SUPER       0
#define   SFS_BLKN_BLMAP       1
#define   SFS_BLKN_INMAP       2
#define   SFS_BLKN_INTABLE     3   //(1024* 128byte) / 4096 = 32
#define   SFS_BLKN_ROOT        35
#define   SFS_BLKN_DATA        36



#define SFS_TYPE_DIR   0
#define SFS_TYPE_FILE  1
#define SFS_TYPE_CHAR  2

struct sfs_superblock{
	unsigned int   magic;      //magic number
	unsigned int   ublocks;    
	unsigned int   uinodes;
	unsigned int   blocks;
	unsigned int   inodes;
};

struct sfs_inode{
	unsigned int  i_inode;
	unsigned int  i_type;
	unsigned int  i_size;
	unsigned int  i_nlinks;
	unsigned int  i_blocks;
	unsigned int  i_direct[12];
             int  i_indirect;
}__attribute__((aligned(128)));

struct dir_entry{
    unsigned int   inode_number;
    char           dir_name[SFS_MAX_FNAME_LEN + 1];
};


struct sfs_blockbitmap{
	unsigned  int   nbits;
	unsigned  int   nwords;
	unsigned  int   *map;
};


//内存中始终维护一个数据结构
struct sfs_mem_ctrl{
	struct  sfs_superblock    *superblock;
	struct  sfs_blockbitmap   *blockmap;
	char    *inodemap;
    struct  sfs_inode  *inodetable;
	struct  dir_entry  *root;    //将root节点的信息给它
	int inodetablelength;        //内存当中当前载入inode 的多少
};
extern struct sfs_mem_ctrl *Ctrl;
#endif //__MKSFS_H__
