#include "newfs.h"

void newfs_copy_dentry(struct newfs_dentry *dentry, char *name, int ino, FILE_TYPE ftype) {
    memset(dentry->name, 0, 10);
    strcpy(dentry->name, name);
    dentry->ino = ino;
    dentry->type = ftype;
    dentry->valid = 1;
}

void newfs_get_fname(char *fname, const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch);
    *q = 0;
    strcpy(fname, ++q);
}

int blk_read(int fd, int blk, int nums, char *buf)
{
    if(nums == 0) return 0;
    int ret = 0;
    for(int i = 0; i < nums; ++i)
    {
        if(ddriver_seek(fd, (blk + i) * BLK_SIZE, SEEK_SET) < 0)
        {
            printf("seek error!\n");
            exit(1);
        }
        if(ddriver_read(fd, buf, BLK_SIZE) == 0)
        {
            printf("read error!\n");
            exit(1);
        }
        buf += BLK_SIZE;
        ++ret;
    }
    return ret;
}

int blk_write(int fd, int blk, int nums, char *buf)
{
    if(nums == 0) return 0;
    int ret = 0;
    for(int i = 0; i < nums; ++i)
    {
        if(ddriver_seek(fd, (blk + i) * BLK_SIZE, SEEK_SET) < 0)
        {
            printf("error: seek error!\n");
            exit(1);
        }
        if(ddriver_write(fd, buf, BLK_SIZE) == 0)
        {
            printf("read error!\n");
            exit(1);
        }
        buf += BLK_SIZE;
        ++ret;      
    }
    return ret;
}

int newfs_calc_lvl(const char *path)
{
    char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}

void root_init()
{
    struct newfs_inode root_inode;
    memset(&root_inode, 0, sizeof(struct newfs_inode));
    root_inode.ino = INODE_BLK_OFFSET;
    root_inode.size = 1;
    root_inode.type = DIR;
    char buf[BLK_SIZE];
    memcpy(buf, &root_inode, sizeof(struct newfs_inode));

    blk_write(super.fd, INODE_BLK_OFFSET, 1, buf);
    super.map_inode[0] |= (1 << 0);
}

int newfs_alloc_inode(FILE_TYPE type) {
    int offset = 0;
    int is_find_free_entry = 0;

    for (int i=0; i<super.max_ino/CHAR; i++) {
        for (int j=0; j<CHAR; j++) {
            if ((super.map_inode[i] & (0x1 << j)) == 0) {
                super.map_inode[i] |= (0x1 << j);
                is_find_free_entry = 1;
                break;
            }
            offset++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || offset == super.max_ino) {
        return 0;
    }

    struct newfs_inode ino = {0};
    char buf[BLK_SIZE] = {0};
    ino.ino = INODE_BLK_OFFSET + offset;
    ino.size = 0;
    ino.type = type;
    memcpy(buf, &ino, sizeof(struct newfs_inode));
    blk_write(super.fd, INODE_BLK_OFFSET + offset, 1, buf);
    return INODE_BLK_OFFSET + offset;
}

int newfs_alloc_data()
{
    char buf[2 * BLK_SIZE] = {0};
    int offset = 0;
    int is_find_free_entry = 0;
    for (int i=0; i<super.max_ino/CHAR; i++) {
        for (int j=0; j<CHAR; j++) {
            if ((super.map_data[i] & (0x1 << j)) == 0) {
                super.map_data[i] |= (0x1 << j);
                is_find_free_entry = 1;
                break;
            }
            offset++;
        }
        if (is_find_free_entry){
            break;
        }
    }

    if (!is_find_free_entry || offset == super.max_data) {
        return -1;
    }
    blk_write(super.fd, super.blks_data_offset + offset*2, 2, buf);
    return super.blks_data_offset + offset * 2;
}

int newfs_alloc_dentry(char *name, int ino, FILE_TYPE type, struct newfs_inode *inode)
{
    struct newfs_dentry dentry;
    char buf[2 * BLK_SIZE];
    for(int i = 0; i < 6; ++i) {
        if(inode->blk_pointer[i] <= 0) {
            inode->blk_pointer[i] = newfs_alloc_data();
        }
        blk_read(super.fd, inode->blk_pointer[i], 2, buf);
        for (int j=0; j<(2*BLK_SIZE)/sizeof(struct newfs_dentry); j++) {
            memcpy(&dentry, buf + j * sizeof(struct newfs_dentry), sizeof(struct newfs_dentry));
            if (!dentry.valid) {
                newfs_copy_dentry(&dentry, name, ino, type);
                inode->dentrys[inode->dir_cnt] = dentry;
                inode->dir_cnt++;
                memcpy(buf + j * sizeof(struct newfs_dentry), &dentry, sizeof(struct newfs_dentry));
                blk_write(super.fd, inode->blk_pointer[i], 2, buf);
                return 0;
            }
        }
    }
    return -1;
}

struct newfs_dentry *newfs_get_dentry(struct newfs_inode *inode, const char *fname, FILE_TYPE type, int mode)
{
    struct newfs_dentry *dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    int cnt = inode->dir_cnt;
    if (inode->type == REG) {
        return 0;
    }
    while (cnt > 0) {
        memcpy(dentry, (struct newfs_dentry*)&inode->dentrys[--cnt], sizeof(struct newfs_dentry));
        if (!strcmp(dentry->name, fname) && (dentry->type == type || !mode)) {
            return dentry;
        }
    }
    free(dentry);
    return 0;
}

struct newfs_dentry *newfs_dir_dentry(int ino, off_t off)
{
    char buf[BLK_SIZE];
    struct newfs_inode inode;
    blk_read(super.fd, ino, 1, buf);
    memcpy(&inode, buf, sizeof(struct newfs_inode));

    if(inode.type == REG || off >= inode.dir_cnt) {
        return 0;
    }

    struct newfs_dentry *dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    memcpy(dentry, (struct newfs_dentry*)&inode.dentrys[off], sizeof(struct newfs_dentry));
    return dentry;
}

struct newfs_dentry *newfs_lookup(const char *_path, mode_t mode)
{
    char path[BLK_SIZE] = {0};
    char buf[BLK_SIZE] = {0};
    struct newfs_inode cur_inode;
    struct newfs_dentry *cur_dentry = NULL;
    int total_lvl = newfs_calc_lvl(_path);
    int lvl = 0;
    char *fname = NULL;

    strcpy(path, _path);
    blk_read(super.fd, INODE_BLK_OFFSET, 1, buf);
    memcpy(&cur_inode, buf, sizeof(struct newfs_inode));
    fname = strtok(path, "/");
    while(fname) {
        lvl++;
        cur_dentry = newfs_get_dentry(&cur_inode, fname, DIR, mode);
        if(!cur_dentry) {
            if(!mode) {
                break;
            }
            else {
                cur_dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
                int dir_ino = newfs_alloc_inode(DIR);
                if(dir_ino == 0 || newfs_alloc_dentry(fname, dir_ino, DIR, &cur_inode)) {
                    return 0;
                }
                newfs_copy_dentry(cur_dentry, fname, dir_ino, DIR);
                memcpy(buf, &cur_inode, sizeof(struct newfs_inode));
                blk_write(super.fd, cur_inode.ino, 1, buf);
            }
        }
        if(lvl == total_lvl) {
            return cur_dentry;
        }
        else if(cur_dentry->type == DIR) {
            blk_read(super.fd, cur_dentry->ino, 1, buf);
            memcpy(&cur_inode, buf, sizeof(struct newfs_inode));
            fname = strtok(NULL, "/");
            free(cur_dentry);
        }
        else {
            free(cur_dentry);
            break;
        }
    }
    return 0;
}
