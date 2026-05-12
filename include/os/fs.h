#ifndef __INCLUDE_OS_FS_H__
#define __INCLUDE_OS_FS_H__

#include <type.h>

/* macros of file system */
/*======文件系统基本协议======*/
#define FS_START_SECTOR (1<<20) // 512MB / 512B
#define FS_SIZE (512<<20) // 512MB
#define SECTOR_SIZE 512
#define BLOCK_SIZE 4096 // 4KB
#define BLOCK_SIZE_SHIFT 12
#define SECTORS_PER_BLOCK (BLOCK_SIZE / SECTOR_SIZE)
/*======Super Block======*/
#define SUPERBLOCK_MAGIC 0xDF4C4459
#define SUPERBLOCK_SECTORS 1
#define NUM_FDESCS 16
/*======Block Map======*/
// 使用 block map 位图：512MB / 4KB = 128K块   
// 128K bit = 16 KB = 32 个扇区
#define FS_BLOCK_NUM (FS_SIZE / BLOCK_SIZE)
#define BLOCK_MAP_SECTORS (FS_BLOCK_NUM / 8 / SECTOR_SIZE)
/*======Inode Map======*/
#define NUM_INODES 4096 // 预设最多有 4096 个inode
#define INODE_MAP_SECTORS (NUM_INODES / 8 / SECTOR_SIZE) // 4096 bit = 512 B = 1 sector
/*======Inode======*/
// 存放inode信息表的空间
#define INODE_TABLE_SECTORS (NUM_INODES * sizeof(inode_t) / SECTOR_SIZE)
typedef enum {
    FD_FILE,  // 普通文件
    FD_DIR,   // 目录
} file_type_t;
typedef enum {
    MODE_READ    = 1<<0,
    MODE_WRITE   = 1<<1,
    MODE_EXEC    = 1<<2,
} file_mode_t;
#define ROOT_DIR_BLOCK 1
#define ROOT_INODE_NUM 0
#define MAX_PATH_LEN 80 // 最大路径长度
/*======Data======*/
#define DATA_SECTORS (FS_SIZE - SUPERBLOCK_SECTORS \
    - BLOCK_MAP_SECTORS - INODE_MAP_SECTORS - INODE_TABLE_SECTORS)
#define BLOCK_BASE (FS_START_SECTOR + SUPERBLOCK_SECTORS + \
        INODE_MAP_SECTORS + BLOCK_MAP_SECTORS + INODE_TABLE_SECTORS)

/*======Dentry======*/
#define MAX_FNAME_LEN 16 // 文件名最大长度 + 1
#define DIRECT_BLOCK_NUM 10 // 直接索引支持的最大文件大小为 DIRECT_BLOCK_NUM * BLOCK_SIZE = 36KB
#define INDIRECT_BLOCK_NUM (BLOCK_SIZE / sizeof(block_idx_t)) // 一级间接索引支持的文件大小 (块数)
#define DOUBLE_INDIRECT_BLOCK_NUM (INDIRECT_BLOCK_NUM * INDIRECT_BLOCK_NUM)
#define TRIPLE_INDIRECT_BLOCK_NUM (INDIRECT_BLOCK_NUM * DOUBLE_INDIRECT_BLOCK_NUM)


typedef uint32_t block_idx_t;

/* data structures of file system */
typedef struct superblock {
    // TODO [P6-task1]: Implement the data structure of superblock
    uint64_t magic;
    uint64_t fs_size; // 文件系统大小（扇区数）
    uint64_t start_sector; // 起始扇区
    uint64_t block_size;

    // 各区域偏移量（扇区数）
    uint64_t inode_map_offset;
    uint64_t block_map_offset;
    uint64_t inode_offset;
    uint64_t data_offset;

    uint64_t inode_entry_size;
    uint64_t dentry_size;
} superblock_t;

typedef struct dentry {
    // TODO [P6-task1]: Implement the data structure of directory entry
    char name[MAX_FNAME_LEN];
    uint32_t inode_num; // 该文件对应的 inode 编号
} dentry_t;

typedef struct inode { 
    // TODO [P6-task1]: Implement the data structure of inode
    file_mode_t mode; // 文件权限 使用 onehot 编码 rwx
    file_type_t type;
    uint32_t size; // 文件大小（字节数）
    uint32_t nlinks; // 硬链接数

    block_idx_t direct_ptr[DIRECT_BLOCK_NUM]; // 直接索引：指向数据块索引（起始于data区）
    block_idx_t indirect_ptr;                 // 一级间接寻址
    block_idx_t double_indirect_ptr;          // 二级间接寻址
} __attribute__((aligned(64)))inode_t;

typedef struct fdesc {
    // TODO [P6-task2]: Implement the data structure of file descriptor
    uint32_t inode_num; // 该描述符关联的 inode 编号
    uint32_t mode;      // 打开权限（O_RDONLY, O_WRONLY, O_RDWR）
    uint32_t pos;       // 当前读写位置的偏移量
    uint32_t used;      // 标记此文件描述符槽位是否被占用
} fdesc_t;

/* modes of do_open */
#define O_RDONLY 1  /* read only open */
#define O_WRONLY 2  /* write only open */
#define O_RDWR   3  /* read/write open */

/* whence of do_lseek */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* fs function declarations */
extern void init_fs();
extern int do_mkfs(void);
extern int do_statfs(void);
extern int do_cd(char *path);
extern int do_mkdir(char *path);
extern int do_rmdir(char *path);
extern int do_ls(char *path, int option);
extern int do_touch(char *path);
extern int do_open(char *path, int mode);
extern int do_read(int fd, char *buff, int length);
extern int do_write(int fd, char *buff, int length);
extern int do_close(int fd);
extern int do_ln(char *src_path, char *dst_path);
extern int do_rm(char *path);
extern int do_lseek(int fd, int offset, int whence);

/* tool functions */
void read_sb(superblock_t *sb);
void write_sb(superblock_t *sb);
void read_block(uint32_t block_id, void* buf);
void write_block(uint32_t block_id, void* buf);
void get_inode(uint32_t inode_num, inode_t *node);
void write_inode(uint32_t inode_num, inode_t *node);
uint32_t alloc_inode();
void free_inode(uint32_t inode_num);
uint32_t alloc_block();
void free_block(uint32_t block_num);
uint32_t find_inode_by_name(uint32_t dir_inode_num, char *name);
uint32_t find_inode_by_path(char *path);
void append_dentry(inode_t* dir_inode, uint32_t dir_inode_num, dentry_t dentry);
void remove_dentry(inode_t* p_inode, uint32_t p_inode_num, char *name);
int alloc_fd();
uint32_t get_physical_block_id(inode_t *node, uint32_t inode_num, uint32_t logical_id, int alloc);

extern superblock_t sb;
extern int sb_valid;
static fdesc_t fdesc_array[NUM_FDESCS];
#endif