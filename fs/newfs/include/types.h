#ifndef _TYPES_H_
#define _TYPES_H_

#define MAX_NAME_LEN    10     
#define BLK_SIZE        512     //size of a block

#define CHAR            8

#define FILE_MAX        20     //max file nums

#define INODE_BLK_NUM   100      //nums of inode blk
#define INODE_BLK_OFFSET 3
#define MAX_INODE_SIZE  108     //len of map_inode

#define DATA_START_OFF  3 + INODE_BLK_NUM    //data start blk
#define MAX_DATA_SIZE    FILE_MAX * 12 + 10     // max num of data_blk
#define MAP_DATA_SIZE   108      //len of map_inode

#define MAX_DENTRYS     6

struct custom_options {
	const char*        device;
};

struct newfs_super {
    
    uint32_t magic;
    int      fd;

    int max_file;        //max num of file

    int map_inode_blks;        //max num of inode bmap blks
    int map_inode_offset;     //offset of inode map

    int max_ino;        // max num of inode

    int map_data_blks;  //max numb of map_data
    int map_data_offset;  //offset of map_data

    int max_data;       //numbers of data blk
    int blks_data_offset;       //offset of data

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
    /* TODO: Define yourself */
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