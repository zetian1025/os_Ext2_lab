#ifndef _TYPES_H_
#define _TYPES_H_

#define MAX_NAME_LEN        10     
#define BLK_SIZE            512
#define CHAR                8
#define INODE_BLK_OFFSET    3
#define MAX_INODE_SIZE      108
#define DATA_START_OFF      3 + MAX_INODE_SIZE
#define MAX_DATA_SIZE       MAX_INODE_SIZE * 12
#define MAX_DENTRYS         6

struct custom_options {
	const char*        device;
};

struct newfs_super {
    uint32_t    magic;
    int         fd;
    int         is_init;        //max num of file
    int         max_ino;        // max num of inode
    int         max_data;       //numbers of data blk
    char*       map_inode;
    char*       map_data;
    int         blks_data_offset;       //offset of data
};

typedef enum file_type{
    DIR,
    REG
} FILE_TYPE;

struct newfs_dentry {
    char     name[MAX_NAME_LEN];
    uint32_t ino;
    FILE_TYPE type;     // the type of inofille which point to
    int valid;          // the valid of inofile
};

struct newfs_inode {
    uint32_t ino;       // inode's index in ino bmap
    int size;           // file's size
    FILE_TYPE type;     // file's type: dir or file
    int dir_cnt;        // if it's dir, numbers of dirfile count
    int blk_pointer[6]; // the offset of file
    struct newfs_dentry dentrys[MAX_DENTRYS];
};


#endif /* _TYPES_H_ */