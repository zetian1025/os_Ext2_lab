#ifndef _NEWFS_H_
#define _NEWFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define NEWFS_MAGIC           0x1910715       /* TODO: Define by yourself */
#define NEWFS_DEFAULT_PERM    0777   /* 全权限打开 */

struct newfs_super super; 
/******************************************************************************
* SECTION: newfs.c
*******************************************************************************/
void* 			   newfs_init(struct fuse_conn_info *);
void  			   newfs_destroy(void *);
int   			   newfs_mkdir(const char *, mode_t);
int   			   newfs_getattr(const char *, struct stat *);
int   			   newfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   newfs_mknod(const char *, mode_t, dev_t);
int   			   newfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   newfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   newfs_access(const char *, int);
int   			   newfs_unlink(const char *);
int   			   newfs_rmdir(const char *);
int   			   newfs_rename(const char *, const char *);
int   			   newfs_utimens(const char *, const struct timespec tv[2]);
int   			   newfs_truncate(const char *, off_t);
			
int   			   newfs_open(const char *, struct fuse_file_info *);
int   			   newfs_opendir(const char *, struct fuse_file_info *);
/******************************************************************************
* SECTION: util.c
*******************************************************************************/
int blk_read(int fd, int blk, int nums, char *buf);
int blk_write(int fd, int blk, int nums, char *buf);

void root_init();

void newfs_copy_dentry(struct newfs_dentry *dentry, char *name, int ino, FILE_TYPE ftype);

void newfs_get_dentrys(struct newfs_inode *inode);

int newfs_alloc_inode(FILE_TYPE type);

struct newfs_dentry *newfs_get_dentry(struct newfs_inode *ino, const char *fname, FILE_TYPE type, int create);

struct newfs_dentry *newfs_lookup(const char *_path, mode_t mode);

int newfs_alloc_dentry(char *name, int ino, FILE_TYPE type, struct newfs_inode *inode);

int newfs_alloc_data();

struct newfs_dentry *newfs_dir_dentry(int ino, off_t off);

void newfs_get_fname(char *fname, const char* path);

#endif  /* _newfs_H_ */