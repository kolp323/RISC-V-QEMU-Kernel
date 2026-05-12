#include <os/string.h>
#include <os/fs.h>
#include <printk.h>
#include <os/kernel.h>
#include <os/sched.h>


static fdesc_t fdesc_array[NUM_FDESCS];
// static uint8_t super_block_buffer[SECTOR_SIZE];

superblock_t sb; // 文件系统信息的全局记录
int sb_valid = 0; 

void init_fs() {
    read_sb(&sb);
    if (sb.magic == SUPERBLOCK_MAGIC) {
        printk("[FS] Found existing file system.\n");
        sb_valid = 1;
    } else {
        printk("[FS] No file system detected. Auto-initializing...\n");
        do_mkfs();
    }

    for (int i = 0; i < NUM_FDESCS; i++) {
        fdesc_array[i].pos = 0;
        fdesc_array[i].used = 0;
    }
}

int do_mkfs(void)
{
    // TODO [P6-task1]: Implement do_mkfs
	printk("[FS] Start initialize filesystem!\n");
    
    // 初始化 Super Block
    // superblock_t sb;
    read_sb(&sb);
    if (sb.magic == SUPERBLOCK_MAGIC) {
        printk("[FS] A file system has already existed!\n");
        sb_valid = 1;
        return -1;
    }
    printk("[FS] Setting superblock...\n");
    sb.magic = SUPERBLOCK_MAGIC;
    sb.fs_size = FS_SIZE / SECTOR_SIZE; // 总扇区数
    sb.block_size = BLOCK_SIZE; // 数据块大小
    sb.start_sector = FS_START_SECTOR;
    sb.inode_map_offset = SUPERBLOCK_SECTORS; // 紧跟 SB
    sb.block_map_offset = sb.inode_map_offset + INODE_MAP_SECTORS;
    sb.inode_offset = sb.block_map_offset + BLOCK_MAP_SECTORS;
    sb.data_offset = sb.inode_offset + INODE_TABLE_SECTORS;
    sb.inode_entry_size = sizeof(inode_t);
    sb.dentry_size = sizeof(dentry_t);
    write_sb(&sb);
    sb_valid = 1; // 全局记录有效
	printk("     magic number: 0x%lx\n",sb.magic);
	printk("     num sector: %ld, start sector: %ld\n", sb.fs_size, sb.start_sector);
    printk("     num block: %ld, block size: %ld\n", sb.fs_size/SECTORS_PER_BLOCK, sb.block_size);
	printk("     inode map offset: %ld(%ld)\n", sb.inode_map_offset, INODE_MAP_SECTORS);
    printk("     block map offset: %ld(%ld)\n", sb.block_map_offset, BLOCK_MAP_SECTORS);
	printk("     inode offset: %ld(%ld)\n", sb.inode_offset, INODE_TABLE_SECTORS);
	printk("     data offset: %ld(%ld)\n", sb.data_offset, DATA_SECTORS);
	printk("     inode entry size: %ld, dir entry size: %ld\n", sb.inode_entry_size, sb.dentry_size);
	

    // 清空 Inode Map 和 Block Map
    printk("[FS] Setting block-map...\n");
    printk("[FS] Setting inode-map...\n");

    uint8_t *sector_buf = (uint8_t *)TEMP_BUF;
    memset(sector_buf, 0, SECTOR_SIZE);
    for (int i = 0; i < INODE_MAP_SECTORS + BLOCK_MAP_SECTORS; i++) {
        bios_sd_write(
            kva2pa((unsigned long)sector_buf), 
            1, 
            sb.start_sector + sb.inode_map_offset + i);
    }

    printk("[FS] Setting inode...\n");
    // 初始化根目录 inode
    inode_t* root_inode = (inode_t*)TEMP_BUF;
    memset(root_inode, 0, SECTOR_SIZE);
    root_inode->type = FD_DIR;
    root_inode->size = BLOCK_SIZE;
    root_inode->nlinks = 2; // . 和 ..
    root_inode->direct_ptr[0] = ROOT_DIR_BLOCK; // 第 0 个数据块的index
    write_inode(ROOT_INODE_NUM, root_inode);
    // bios_sd_write(
    //     kva2pa((unsigned long)root_inode), 
    //     1, 
    //     sb.start_sector + sb.inode_offset
    // );

    uint8_t* block_buf = (uint8_t *)TEMP_BUF;
    memset(block_buf, 0, BLOCK_SIZE);
    write_block(0, block_buf); // 清空哨兵块
    // bios_sd_write(
    //     kva2pa((unsigned long)block_buf), 
    //     SECTORS_PER_BLOCK, 
    //     sb.start_sector + sb.data_offset
    // ); // 清空哨兵块

    // 初始化根目录数据块（写入.和..）
    dentry_t *dentries = (dentry_t *)block_buf;
    strcpy(dentries[0].name, ".");
    dentries[0].inode_num = ROOT_INODE_NUM;
    strcpy(dentries[1].name, "..");
    dentries[1].inode_num = ROOT_INODE_NUM;
    write_block(ROOT_DIR_BLOCK, block_buf);
    // bios_sd_write(
    //     kva2pa((unsigned long)block_buf), 
    //     SECTORS_PER_BLOCK, 
    //     sb.start_sector + sb.data_offset + ROOT_DIR_BLOCK
    // );

    // 更新 block map 和 inode map
    uint8_t* map_buf = (uint8_t *)TEMP_BUF;
    memset(map_buf, 0, SECTOR_SIZE);
    map_buf[0] = 0b1; // 只占用了根目录inode
    bios_sd_write(
        kva2pa((unsigned long)map_buf), 
        1, 
        sb.start_sector + sb.inode_map_offset);
    map_buf[0] = 0b11; // 第0个数据块作为哨兵 + 根目录block
    bios_sd_write(
        kva2pa((unsigned long)map_buf), 
        1, 
        sb.start_sector + sb.block_map_offset);
    printk("[FS] Initialize filesystem finished!\n");
    return 0;
}

int do_statfs(void)
{
    // TODO [P6-task1]: Implement do_statfs
    // superblock_t sb;
    if (!sb_valid) {
        read_sb(&sb);
        sb_valid = 1;
    }
    if (sb.magic != SUPERBLOCK_MAGIC) {
        printk("[FS] Error: No file system found!\n");
        return -1;
    }

    // 统计已用 Inode
    uint32_t used_inodes = 0;
    uint8_t map_buf[SECTOR_SIZE];
    for (int s = 0; s < INODE_MAP_SECTORS; s++) {
        bios_sd_read(
        kva2pa((unsigned long)map_buf), 
        1, 
        sb.start_sector + sb.inode_map_offset + s
        );   
        for (int i = 0; i < SECTOR_SIZE; i++) {
            for (int bit = 0; bit < 8; bit++) {
                if (map_buf[i] & (1<<bit)) used_inodes++;
            }
        }
    }

    // 统计已用 Block
    uint32_t used_blocks = 0;
    for (int s = 0; s < BLOCK_MAP_SECTORS; s++) {
        bios_sd_read(
            kva2pa((unsigned long)map_buf), 
            1, 
            sb.start_sector + sb.block_map_offset + s
        );
        for (int i = 0; i < SECTOR_SIZE; i++) {
            for (int bit = 0; bit < 8; bit++) {
                if (map_buf[i] & (1<<bit)) used_blocks++;
            }
        }
    }
    printk("magic: 0x%lx (KFS)\n", sb.magic);
    printk("used sector: %ld/%ld, start sector: %ld\n", 
           used_blocks * SECTORS_PER_BLOCK + sb.data_offset, sb.fs_size, sb.start_sector);
    printk("inode map offset: %ld, occupied sector: %d, used: %d/%d\n", 
        sb.inode_map_offset, INODE_MAP_SECTORS, used_inodes, NUM_INODES);
    printk("block map offset: %ld, occupied sector: %d, used: %d/%d\n", 
        sb.block_map_offset, BLOCK_MAP_SECTORS, used_blocks, sb.fs_size/SECTORS_PER_BLOCK);
	printk("inode offset: %ld, occupied sector: %d\n", sb.inode_offset, INODE_TABLE_SECTORS);
	printk("data offset: %ld, occupied sector: %d\n", sb.data_offset, DATA_SECTORS);
	printk("inode entry size: %ld, dir entry size: %ld\n", sb.inode_entry_size, sb.dentry_size);

    return 0;  // do_statfs succeeds
}

int do_cd(char *path)
{
    // TODO [P6-task1]: Implement do_cd
    if (path == NULL) return 0;
    uint32_t target_inode = find_inode_by_path(path);
    if (target_inode == -1) {
        printk("[cd]: No such file or directory: %s\n", path);
        return -1;
    }

    inode_t node;
    get_inode(target_inode, &node);
    if (node.type != FD_DIR) {
        printk("[cd]: Not a directory: %s\n", path);
        return 1; // 表述输入是文件
    }

    current_running->dir_inode = target_inode;
    return 0;  // do_cd succeeds
}

// /a/b/c
// ./c
// c
int do_mkdir(char *path)
{
    if(path == NULL){
        printk("[FS] warning: mkdir path is NULL!");
        return -1;
    }
    // TODO [P6-task1]: Implement do_mkdir
    uint32_t parent_inode_num;
    char *new_dir_name;
    char path_copy[MAX_PATH_LEN];
    strcpy(path_copy, path);
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) { // 一级路径
        parent_inode_num = current_running->dir_inode;
        new_dir_name = path;
    }else{ // 多级路径
        *last_slash = '\0'; // 截断
        new_dir_name = last_slash + 1;
        parent_inode_num = find_inode_by_path(path_copy);
        if (parent_inode_num == -1) { 
            printk("[FS] mkdir: Parent directory not found!\n");
            return -1;
        }
    }

    // 防止重名
    if (find_inode_by_name(parent_inode_num, new_dir_name) != -1) return -1;

    uint32_t new_inode_num = alloc_inode();
    uint32_t new_block_num = alloc_block();
    // 填入新目录的 inode 表项
    inode_t new_inode;
    memset(&new_inode, 0, sizeof(inode_t));
    new_inode.type = FD_DIR;
    new_inode.size = BLOCK_SIZE;
    new_inode.nlinks = 2; // 自己和父目录的引用
    new_inode.direct_ptr[0] = new_block_num;
    write_inode(new_inode_num, &new_inode);

    // 初始化新数据块目录项: 写入 . 和 ..
    uint8_t* buf = (uint8_t*)TEMP_BUF;
    memset(buf, 0, BLOCK_SIZE);
    dentry_t *d = (dentry_t *)buf;
    strcpy(d[0].name, "."); d[0].inode_num = new_inode_num;
    strcpy(d[1].name, ".."); d[1].inode_num = parent_inode_num;
    write_block(new_block_num, buf);

    // 更新父目录数据块目录项 和 inode信息
    dentry_t new_d;
    strcpy(new_d.name, new_dir_name);
    new_d.inode_num = new_inode_num;
    inode_t parent_node;
    get_inode(parent_inode_num, &parent_node);
    append_dentry(&parent_node, parent_inode_num, new_d);
    parent_node.nlinks++; // 新目录的..
    write_inode(parent_inode_num, &parent_node); // 写回更新的 inode

    return 0;  // do_mkdir succeeds
}

int do_rmdir(char *path)
{
    // TODO [P6-task1]: Implement do_rmdir
    char path_copy[MAX_PATH_LEN];
    strcpy(path_copy, path);
    uint32_t parent_inode_num;
    char *target_name;
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) {
        parent_inode_num = current_running->dir_inode;
        target_name = path;
    } else {
        *last_slash = '\0';
        target_name = last_slash + 1;
        parent_inode_num = find_inode_by_path(path_copy);
    }

    uint32_t target_inode_num = find_inode_by_name(parent_inode_num, target_name);
    if (target_inode_num == -1) return -1; // 不存在
    if (target_inode_num == ROOT_INODE_NUM) return -1; // 不能删根目录
    inode_t target_node;
    get_inode(target_inode_num, &target_node);
    if (target_node.type != FD_DIR) return -1; // 不是目录

    // 检查目录是否为空
    uint8_t *block_buf = (uint8_t*)TEMP_BUF;
    int dentry_count = 0;
    for (int i = 0; i < DIRECT_BLOCK_NUM; i++) {
        if (target_node.direct_ptr[i] == 0 && i != 0) break;
        read_block(target_node.direct_ptr[i], block_buf);
        dentry_t *d = (dentry_t *)block_buf;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dentry_t); j++) {
            if (d[j].name[0] != '\0') dentry_count++;
        }
    }
    if (dentry_count > 2) {
        printk("[FS] Error: Directory not empty!\n");
        return -1;
    }

    inode_t parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    // 从父目录移除目录项
    remove_dentry(&parent_inode, parent_inode_num, target_name);
    // 释放资源
    for (int i = 0; i < DIRECT_BLOCK_NUM; i++) {
        if (target_node.direct_ptr[i] != 0 || i == 0)
            free_block(target_node.direct_ptr[i]);
    }
    free_inode(target_inode_num);

    // 更新父目录状态
    parent_inode.nlinks--;
    write_inode(parent_inode_num, &parent_inode);

    return 0;  // do_rmdir succeeds
}

int do_ls(char *path, int option)
{
    // TODO [P6-task1]: Implement do_ls
    // Note: argument 'option' serves for 'ls -l' in A-core
    uint32_t target_inode;
    if (path == NULL || strlen(path) == 0) target_inode = current_running->dir_inode;
    else target_inode = find_inode_by_path(path);
    if (target_inode == -1) {
        printk("[ls]: Cannot access %s\n", path);
        return -1;
    }

    inode_t dir_node;
    get_inode(target_inode, &dir_node);
    if (dir_node.type != FD_DIR) {
        // TODO: 处理路径末端是文件的情况，应该能正常输出文件名
        char* name = path + strlen(path);
        while (name != path && *name != '/') name--;
        if (*name == '/') printk("%s\n", name+1); // a/b
        else printk("%s\n", name); // b
        return 0;
    }

    uint8_t *block_buf = (uint8_t *)TEMP_BUF;
    uint32_t num_blocks = (dir_node.size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    for (uint32_t i = 0; i < num_blocks; i++) {
        uint32_t p_block = get_physical_block_id(&dir_node, target_inode, i, 0); //
        if (p_block == -1 || p_block == 0) continue;
        read_block(p_block, block_buf);
        dentry_t *dentries = (dentry_t *)block_buf;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dentry_t); j++) {
            if (dentries[j].name[0] != '\0') {
                if (option == 1) { // ls -l 
                    inode_t tmp_node;
                    get_inode(dentries[j].inode_num, &tmp_node);
                    printk("%s\t inode: %d\t nlinks: %d\t size: %d\n", dentries[j].name, dentries[j].inode_num, tmp_node.nlinks, tmp_node.size);
                } else {
                    printk("%s  ", dentries[j].name);
                }
            }
        }
    }
    printk("\n");
    return 0;  // do_ls succeeds
}

// 创建成功时，返回新建文件的inode_num
int do_touch(char *path) {
    if(path == NULL){
        printk("[FS] warning: touch path is NULL!");
        return -1;
    }

    uint32_t parent_inode_num;
    char *new_file_name;
    char path_copy[MAX_PATH_LEN];
    strcpy(path_copy, path);
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) { // 一级路径
        parent_inode_num = current_running->dir_inode;
        new_file_name = path;
    }else{ // 多级路径
        *last_slash = '\0'; // 截断
        new_file_name = last_slash + 1;
        parent_inode_num = find_inode_by_path(path_copy);
        if (parent_inode_num == -1) { 
            printk("[FS] touch: Parent directory not found!\n");
            return -1;
        }
    }
    // 防止重名
    if (find_inode_by_name(parent_inode_num, new_file_name) != -1) return -1;

    // 分配新 inode
    uint32_t new_inode_num = alloc_inode();
    // 填入新文件的 inode 表项
    inode_t new_inode;
    memset(&new_inode, 0, sizeof(inode_t));
    new_inode.type = FD_FILE;
    new_inode.size = 0;
    new_inode.nlinks = 1; // 父目录的引用
    write_inode(new_inode_num, &new_inode);

    // 更新父目录数据块目录项 和 inode信息
    dentry_t new_d;
    strcpy(new_d.name, new_file_name);
    new_d.inode_num = new_inode_num;
    inode_t parent_node;
    get_inode(parent_inode_num, &parent_node);
    append_dentry(&parent_node, parent_inode_num, new_d);
    write_inode(parent_inode_num, &parent_node); // 写回更新的 inode
    return new_inode_num;
}

int do_open(char *path, int mode)
{
    // TODO [P6-task2]: Implement do_open
    uint32_t inode_num = find_inode_by_path(path);
    if (inode_num == -1) {
        // TODO: 如果是写模式且文件不存在，调用 do_touch 创建
        if (mode == O_RDWR || mode == O_WRONLY) {
            inode_num = do_touch(path);
            if(inode_num <= 0) return -1; // 创建失败，终止后续操作
        }else{
            printk("[FS] open: file %s not found\n", path);
            return -1;
        }
    }

    inode_t node;
    get_inode(inode_num, &node);
    if (node.type != FD_FILE) {
        printk("[FS] open: %s is not a file\n", path);
        return -1;
    }

    int fd = alloc_fd();
    if (fd == -1) {
        printk("[FS] open: no available file descriptors\n");
        return -1;
    }

    fdesc_array[fd].inode_num = inode_num;
    fdesc_array[fd].mode = mode;
    fdesc_array[fd].pos = 0; // 默认从头开始读写
    fdesc_array[fd].used = 1;

    return fd;  // return the id of file descriptor
}

int do_read(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_read
    if (fd < 0 || fd >= NUM_FDESCS || fdesc_array[fd].used == 0) return -1;
    fdesc_t *fdesc = &fdesc_array[fd];
    if (fdesc->mode == O_WRONLY) return -1; // 只写文件
    inode_t inode;
    get_inode(fdesc->inode_num, &inode);
    if (fdesc->pos >= inode.size) return 0; // 不能读过文件末尾
    // 内容不足时，只能读取剩余内容
    int remain_in_file = inode.size - fdesc->pos;
    int real_read_len = (length < remain_in_file) ? length : remain_in_file;

    int total_read = 0;
    uint8_t *block_buf = (uint8_t *)TEMP_BUF;
    while (total_read < real_read_len) {
        uint32_t logical_block = fdesc->pos / BLOCK_SIZE;
        uint32_t offset_in_block = fdesc->pos % BLOCK_SIZE;
        uint32_t bytes_left_in_block = BLOCK_SIZE - offset_in_block;
        // 本次循环能读取的长度
        uint32_t bytes_to_copy = (real_read_len - total_read < bytes_left_in_block) ? 
                                 (real_read_len - total_read) : bytes_left_in_block;

        uint32_t physical_block = get_physical_block_id(&inode, fdesc->inode_num, logical_block, 0);
        if (physical_block == -1) break; // 读取失败
        read_block(physical_block, block_buf);
        memcpy((uint8_t *)(buff + total_read), block_buf + offset_in_block, bytes_to_copy);
        total_read += bytes_to_copy;
        fdesc->pos += bytes_to_copy;
    }
    return total_read; // return the length of trully read data
}

int do_write(int fd, char *buff, int length)
{
    // TODO [P6-task2]: Implement do_write
    if (fd < 0 || fd >= NUM_FDESCS || fdesc_array[fd].used == 0) return -1;
    fdesc_t *fdesc = &fdesc_array[fd];
    if (fdesc->mode == O_RDONLY) return -1; // 只读文件

    inode_t inode;
    get_inode(fdesc->inode_num, &inode);

    int total_written = 0;
    uint8_t *block_buf = (uint8_t *)TEMP_BUF;

    while (total_written < length) {
        uint32_t logical_block = fdesc->pos / BLOCK_SIZE;
        uint32_t offset_in_block = fdesc->pos % BLOCK_SIZE;
        uint32_t bytes_left_in_block = BLOCK_SIZE - offset_in_block;
        uint32_t bytes_to_copy = (length - total_written < bytes_left_in_block) ? 
                                 (length - total_written) : bytes_left_in_block;

        // 自动分配
        uint32_t physical_block = get_physical_block_id(&inode, fdesc->inode_num, logical_block, 1);
        if (physical_block == -1) break; // 磁盘空间不足

        read_block(physical_block, block_buf);
        // 修改缓冲区内容并写回磁盘
        memcpy(block_buf + offset_in_block, (uint8_t *)(buff + total_written), bytes_to_copy);
        write_block(physical_block, block_buf);

        total_written += bytes_to_copy;
        fdesc->pos += bytes_to_copy;
        // 更新文件大小
        if (fdesc->pos > inode.size) inode.size = fdesc->pos;
    }

    // 更新 inode
    write_inode(fdesc->inode_num, &inode);
    return total_written; // return the length of trully written data
}

int do_close(int fd)
{
    // TODO [P6-task2]: Implement do_close
    if (fd < 0 || fd >= NUM_FDESCS || fdesc_array[fd].used == 0) {
        return -1;
    }
    fdesc_array[fd].used = 0; // 标记为空闲
    fdesc_array[fd].pos = 0;
    return 0;  // do_close succeeds
}

int do_ln(char *src_path, char *dst_path)
{
    // TODO [P6-task2]: Implement do_ln
    uint32_t src_inode_num = find_inode_by_path(src_path);
    if (src_inode_num == -1) {
        printk("[FS] ln: failed to access '%s': No such file or directory\n", src_path);
        return -1;
    }

    inode_t src_node;
    get_inode(src_inode_num, &src_node);
    if (src_node.type == FD_DIR) {
        printk("[FS] ln: %s: hard link not allowed for directory\n", src_path);
        return -1;
    }

    // 解析目标路径，获取父目录和新文件名
    char path_copy[MAX_PATH_LEN];
    strcpy(path_copy, dst_path);
    uint32_t parent_inode_num;
    char *new_name;
    
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) {
        parent_inode_num = current_running->dir_inode;
        new_name = dst_path;
    } else {
        *last_slash = '\0';
        new_name = last_slash + 1;
        parent_inode_num = find_inode_by_path(path_copy);
    }

    if (parent_inode_num == -1) {
        printk("[FS] ln: failed to create hard link '%s' => '%s': No such file or directory\n", dst_path, src_path);
        return -1;
    }

    // 检查重名
    if (find_inode_by_name(parent_inode_num, new_name) != -1) {
        printk("[FS] ln: failed to create hard link '%s': File exists\n", dst_path);
        return -1;
    }

    // 在目标父目录添加目录项
    dentry_t new_d;
    strcpy(new_d.name, new_name);
    new_d.inode_num = src_inode_num; // 指向src的 inode

    inode_t parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    append_dentry(&parent_inode, parent_inode_num, new_d);
    write_inode(parent_inode_num, &parent_inode);

    // 更新源文件的引用计数并写回
    src_node.nlinks++;
    write_inode(src_inode_num, &src_node);
    return 0;  // do_ln succeeds 
}

int do_rm(char *path)
{
    // TODO [P6-task2]: Implement do_rm
    if(path == NULL){
        printk("[FS] warning: rm path is NULL!");
        return -1;
    }

    char path_copy[MAX_PATH_LEN];
    strcpy(path_copy, path);
    uint32_t parent_inode_num;
    char *target_name;
    
    char *last_slash = strrchr(path_copy, '/');
    if (last_slash == NULL) {
        parent_inode_num = current_running->dir_inode;
        target_name = path;
    } else {
        *last_slash = '\0';
        target_name = last_slash + 1;
        parent_inode_num = find_inode_by_path(path_copy);
    }
    if (parent_inode_num == -1) {
        printk("[FS] rm: cannot remove '%s': No such file or directory\n", path);
        return -1;
    }

    uint32_t target_inode_num = find_inode_by_name(parent_inode_num, target_name);
    if (target_inode_num == -1) {
        printk("[FS] rm: cannot remove '%s': No such file or directory\n", path);
        return -1;
    }

    inode_t target_inode;
    get_inode(target_inode_num, &target_inode);
    if (target_inode.type == FD_DIR) {
        printk("[FS] rm: cannot remove '%s': Is a directory\n", path);
        return -1;
    }

    // 从父目录中移除dentry记录
    inode_t parent_inode;
    get_inode(parent_inode_num, &parent_inode);
    remove_dentry(&parent_inode, parent_inode_num, target_name);
    target_inode.nlinks--; // 减少 inode 表项引用数
    if (target_inode.nlinks == 0) {
        // 引用为0，释放所有数据块
        for (int i = 0; i < DIRECT_BLOCK_NUM; i++) {
            if (target_inode.direct_ptr[i] != 0) {
                free_block(target_inode.direct_ptr[i]);
            }
        }
        // TODO: 释放二级、三级间接索引
        if (target_inode.indirect_ptr != 0) {
            uint32_t *buf = (uint32_t*)TEMP_BUF;
            memset(buf, 0, INDIRECT_BLOCK_NUM * sizeof(block_idx_t));
            read_block(target_inode.indirect_ptr, buf);
            for (int i = 0; i < INDIRECT_BLOCK_NUM; i++) {
                if (buf[i] != 0) free_block(buf[i]);
            }
            free_block(target_inode.indirect_ptr);
        }

        if (target_inode.double_indirect_ptr != 0) {
            uint32_t *buf_l1 = (uint32_t*)TEMP_BUF, *buf_l2 = (uint32_t*)TEMP_BUF + INDIRECT_BLOCK_NUM;
            read_block(target_inode.double_indirect_ptr, buf_l1);
            for (int i = 0; i < INDIRECT_BLOCK_NUM; i++) {
                if (buf_l1[i] != 0) {
                    read_block(buf_l1[i], buf_l2);
                    for (int j = 0; j < INDIRECT_BLOCK_NUM; j++) {
                        if (buf_l2[j] != 0) free_block(buf_l2[j]);
                    }
                    free_block(buf_l1[i]);
                }
            }
            free_block(target_inode.double_indirect_ptr);
        }
        
        // 释放 inode map
        free_inode(target_inode_num);
    } else {
        // 仅更新 inode 信息
        write_inode(target_inode_num, &target_inode);
    }
    return 0;  // do_rm succeeds 
}

int do_lseek(int fd, int offset, int whence)
{
    // TODO [P6-task2]: Implement do_lseek
    if (fd < 0 || fd >= NUM_FDESCS || fdesc_array[fd].used == 0) return -1;
    fdesc_t *fdesc = &fdesc_array[fd];
    inode_t inode;
    get_inode(fdesc->inode_num, &inode);
    uint32_t new_pos;
    switch (whence) {
        case SEEK_SET: new_pos = offset;                break;
        case SEEK_CUR: new_pos = fdesc->pos + offset;   break;
        case SEEK_END: new_pos = inode.size + offset;   break;
        default: return -1; // 无效的 whence
    }
    fdesc->pos = new_pos;
    return fdesc->pos;  // the resulting offset location from the beginning of the file
}
