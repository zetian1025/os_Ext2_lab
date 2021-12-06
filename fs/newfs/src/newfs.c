#include "newfs.h"

/******************************************************************************
* SECTION: 宏定义
*******************************************************************************/
#define OPTION(t, p)        { t, offsetof(struct custom_options, p), 1 }
#define NEWFS_ERROR_NONE          0
/******************************************************************************
* SECTION: 全局变量
*******************************************************************************/
static const struct fuse_opt option_spec[] = {		/* 用于FUSE文件系统解析参数 */
	OPTION("--device=%s", device),
	FUSE_OPT_END
};

struct custom_options newfs_options;			 /* 全局选项 */

/******************************************************************************
* SECTION: FUSE操作定义
*******************************************************************************/
static struct fuse_operations operations = {
	.init = newfs_init,						 /* mount文件系统 */		
	.destroy = newfs_destroy,				 /* umount文件系统 */
	.mkdir = newfs_mkdir,					 /* 建目录，mkdir */
	.getattr = newfs_getattr,				 /* 获取文件属性，类似stat，必须完成 */
	.readdir = newfs_readdir,				 /* 填充dentrys */
	.mknod = newfs_mknod,					 /* 创建文件，touch相关 */
	.write = NULL,								  	 /* 写入文件 */
	.read = NULL,								  	 /* 读文件 */
	.utimens = newfs_utimens,				 /* 修改时间，忽略，避免touch报错 */
	.truncate = NULL,						  		 /* 改变文件大小 */
	.unlink = NULL,							  		 /* 删除文件 */
	.rmdir	= NULL,							  		 /* 删除目录， rm -r */
	.rename = NULL,							  		 /* 重命名，mv */

	.open = NULL,							
	.opendir = NULL,
	.access = NULL
};
/******************************************************************************
* SECTION: 必做函数实现
*******************************************************************************/
/**
 * @brief 挂载（mount）文件系统
 * 
 * @param conn_info 可忽略，一些建立连接相关的信息 
 * @return void*
 */
void* newfs_init(struct fuse_conn_info * conn_info) {
	/* TODO: 在这里进行挂载 */
	char buf[BLK_SIZE];
	
	/* 下面是一个控制设备的示例 */
	
	int fd = ddriver_open("/home/guests/190110715/ddriver");\
	blk_read(fd, 0, 1, buf);
	memcpy(&super, buf, sizeof(struct newfs_super));
	
	if(super.is_init == 0) {
		super.magic = NEWFS_MAGIC;
		super.fd = fd;
		super.is_init = 1;
		super.max_ino = MAX_INODE_SIZE;
		super.max_data = MAX_DATA_SIZE;
		super.blks_data_offset = DATA_START_OFF;
		super.map_inode = (char*)malloc(BLK_SIZE);
		super.map_data = (char*)malloc(BLK_SIZE);
		memset(super.map_inode, 0, BLK_SIZE);
		memset(super.map_data, 0, BLK_SIZE);
		root_init();
	}
	else {
		if(super.magic != NEWFS_MAGIC) {
			exit(1);
		}
		super.map_inode = (char*)malloc(BLK_SIZE);
		super.map_data = (char*)malloc(BLK_SIZE);
		blk_read(fd, 1, 1, super.map_inode);
		blk_read(fd, 2, 1, super.map_data);
	}
	return NULL;
}

/**
 * @brief 卸载（umount）文件系统
 * 
 * @param p 可忽略
 * @return void
 */
void newfs_destroy(void* p) {
	char buf[BLK_SIZE] = {0};
	memcpy(buf, &super, sizeof(struct newfs_super));
	blk_write(super.fd, 0, 1, buf);
	blk_write(super.fd, 1, 1, super.map_inode);
	blk_write(super.fd, 2, 1, super.map_data);
	ddriver_close(super.fd);
	return;
}

/**
 * @brief 创建目录
 * 
 * @param path 相对于挂载点的路径
 * @param mode 创建模式（只读？只写？），可忽略
 * @return int 0成功，否则失败
 */
int newfs_mkdir(const char* path, mode_t mode) {
	if(!strcmp(path, "/")){
		return 0;
	}
	struct newfs_dentry *ret = newfs_lookup(path, 1);
	if(!ret) {
		return -1;
	}
	free(ret);
	return 0;
}
 
/**
 * @brief 获取文件或目录的属性，该函数非常重要
 * 
 * @param path 相对于挂载点的路径
 * @param newfs_stat 返回状态
 * @return int 0成功，否则失败
 */
int newfs_getattr(const char* path, struct stat * newfs_stat) {
	char buf[BLK_SIZE];
	if(!strcmp(path, "/")) {
		blk_read(super.fd, INODE_BLK_OFFSET, 1, buf);
		struct newfs_inode ino;
		memcpy(&ino, buf, sizeof(ino));
		newfs_stat->st_size = ino.dir_cnt * sizeof(struct newfs_dentry);
		newfs_stat->st_mode = S_IFDIR | NEWFS_DEFAULT_PERM;
		newfs_stat->st_nlink  = 2;
	}
	else {
		struct newfs_dentry *dentry = newfs_lookup(path, 0);
		if(dentry == 0) {
			free(dentry);
			return -ENOENT;
		}
		blk_read(super.fd, dentry->ino, 1, buf);
		struct newfs_inode ino;
		memcpy(&ino, buf, sizeof(ino));

		if(dentry->type == DIR) {
			newfs_stat->st_mode = S_IFDIR | NEWFS_DEFAULT_PERM;
			newfs_stat->st_size = ino.dir_cnt * sizeof(struct newfs_dentry);
		}
		else if (dentry->type == REG) {	
			newfs_stat->st_mode = S_IFREG | NEWFS_DEFAULT_PERM;
			newfs_stat->st_size = ino.size;
		}

		newfs_stat->st_nlink = 1;
		newfs_stat->st_uid 	 = getuid();
		newfs_stat->st_gid 	 = getgid();
		newfs_stat->st_atime   = time(NULL);
		newfs_stat->st_mtime   = time(NULL);
		newfs_stat->st_blksize = BLK_SIZE;

		free(dentry);
	}
	return NEWFS_ERROR_NONE;
}

/**
 * @brief 遍历目录项，填充至buf，并交给FUSE输出
 * 
 * @param path 相对于挂载点的路径
 * @param buf 输出buffer
 * @param filler 参数讲解:
 * 
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *				const struct stat *stbuf, off_t off)
 * buf: name会被复制到buf中
 * name: dentry名字
 * stbuf: 文件状态，可忽略
 * off: 下一次offset从哪里开始，这里可以理解为第几个dentry
 * 
 * @param offset 第几个目录项？
 * @param fi 可忽略
 * @return int 0成功，否则失败
 */
int newfs_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset,
			    		 struct fuse_file_info * fi) {
    
	/* TODO: 解析路径，获取目录的Inode，并读取目录项，利用filler填充到buf，可参考/fs/simplefs/sfs.c的sfs_readdir()函数实现 */
    
	int ino;

	if(!strcmp(path, "/")) {
		ino = INODE_BLK_OFFSET;
	}
	else {
		struct newfs_dentry *dentry = newfs_lookup(path, 0);
		if(!dentry) {
			return -ENOENT;
		}
		ino = dentry->ino;
		free(dentry);
	}
	
	struct newfs_dentry *child_dentry = newfs_dir_dentry(ino, offset);
	if(child_dentry != NULL) {
		filler(buf, child_dentry->name, NULL, ++offset);
		free(child_dentry);
	}
	return NEWFS_ERROR_NONE;
}

/**
 * @brief 创建文件
 * 
 * @param path 相对于挂载点的路径
 * @param mode 创建文件的模式，可忽略
 * @param dev 设备类型，可忽略
 * @return int 0成功，否则失败
 */
int newfs_mknod(const char* path, mode_t mode, dev_t dev) {
	char buf[BLK_SIZE] = {0};
	char fname[MAX_NAME_LEN];
	strcpy(buf, path);
	newfs_get_fname(fname, buf);
	
	uint64_t ino = newfs_alloc_inode(REG);
	if(!ino) {
		return -1;
	}

	uint64_t dentry_ino = 0;
	if(!strcmp(fname, path+1)) {
		dentry_ino = INODE_BLK_OFFSET;
	}
	else {
		struct newfs_dentry *dentry = newfs_lookup(buf, 0);
		if(dentry == 0 || dentry->type == REG) {
			free(dentry);
			return -1;
		}
		dentry_ino = dentry->ino;
		free(dentry);
	}

	blk_read(super.fd, dentry_ino, 1, buf);
	struct newfs_inode inode;
	memcpy(&inode, buf, sizeof(struct newfs_inode));

	if(newfs_alloc_dentry(fname, ino, REG, &inode)) {
		return -1;
	}
	memcpy(buf, &inode, sizeof(struct newfs_inode));
	blk_write(super.fd, dentry_ino, 1, buf);
	return 0;
}

/**
 * @brief 修改时间，为了不让touch报错 
 * 
 * @param path 相对于挂载点的路径
 * @param tv 实践
 * @return int 0成功，否则失败
 */
int newfs_utimens(const char* path, const struct timespec tv[2]) {
	(void)path;
	return 0;
}

/******************************************************************************
* SECTION: FUSE入口
*******************************************************************************/
int main(int argc, char **argv)
{
    int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	newfs_options.device = strdup("/home/guests/190110715/ddriver");

	if (fuse_opt_parse(&args, &newfs_options, option_spec, NULL) == -1)
		return -1;
	
	ret = fuse_main(args.argc, args.argv, &operations, NULL);
	fuse_opt_free_args(&args);
	return ret;
}