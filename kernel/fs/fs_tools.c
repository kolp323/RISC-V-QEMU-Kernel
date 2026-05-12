#include <os/fs.h>
#include <os/string.h>
#include <printk.h>
#include <os/kernel.h>
#include <os/sched.h>

// 读取 super_block
void read_sb(superblock_t *sb){
    bios_sd_read(
        kva2pa(TEMP_BUF), 
        SUPERBLOCK_SECTORS, FS_START_SECTOR
    );
    memcpy((uint8_t *)sb, (uint8_t *)TEMP_BUF, sizeof(superblock_t));
}

void write_sb(superblock_t *sb){
    // memset((uint8_t *)TEMP_BUF, 0, SECTOR_SIZE);
    memcpy((uint8_t *)TEMP_BUF, (uint8_t *)sb, sizeof(superblock_t));
    bios_sd_write(
        (unsigned int)kva2pa(TEMP_BUF), 
        SUPERBLOCK_SECTORS, FS_START_SECTOR
    );
}

void read_block(uint32_t block_id, void* buf) {
    bios_sd_read(
        kva2pa((unsigned long)buf), 
        SECTORS_PER_BLOCK, 
        BLOCK_BASE + block_id * SECTORS_PER_BLOCK
    );
}

void write_block(uint32_t block_id, void* buf) {
    bios_sd_write(
        kva2pa((unsigned long)buf), 
        SECTORS_PER_BLOCK, 
        BLOCK_BASE + block_id * SECTORS_PER_BLOCK
    );
}

// 读取指定 inode 表项到 node 中
void get_inode(uint32_t inode_num, inode_t *node){
    uint8_t buffer[SECTOR_SIZE];
    uint32_t inode_table_start = FS_START_SECTOR + SUPERBLOCK_SECTORS + INODE_MAP_SECTORS + BLOCK_MAP_SECTORS;
    uint32_t sector_idx = (inode_num * sizeof(inode_t)) / SECTOR_SIZE;
    uint32_t offset = (inode_num * sizeof(inode_t)) % SECTOR_SIZE;
    bios_sd_read(kva2pa((unsigned long)buffer), 
    1, 
    inode_table_start + sector_idx);
    memcpy((uint8_t *)node, buffer + offset, sizeof(inode_t));
}
// 将inode写回磁盘
void write_inode(uint32_t inode_num, inode_t *node) {
    uint8_t buffer[SECTOR_SIZE];
    uint32_t inode_table_start = FS_START_SECTOR + SUPERBLOCK_SECTORS + INODE_MAP_SECTORS + BLOCK_MAP_SECTORS;
    uint32_t sector_idx = (inode_num * sizeof(inode_t)) / SECTOR_SIZE;
    uint32_t offset = (inode_num * sizeof(inode_t)) % SECTOR_SIZE;

    // 读出，修改目标inode，再写回，防止覆盖其他 inode
    bios_sd_read(kva2pa((unsigned long)buffer), 1, inode_table_start + sector_idx);
    memcpy(buffer + offset, (uint8_t *)node, sizeof(inode_t));
    bios_sd_write(kva2pa((unsigned long)buffer), 1, inode_table_start + sector_idx);
}

// 分配空闲 inode
uint32_t alloc_inode() {
    uint8_t map_buf[SECTOR_SIZE];
    if (!sb_valid) {
        read_sb(&sb);
        sb_valid = 1;
    }
    for (int i = 0; i < INODE_MAP_SECTORS; i++) {
        bios_sd_read(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.inode_map_offset + i);
        for (int j = 0; j < SECTOR_SIZE; j++) {
            if (map_buf[j] != 0xff) { // 该字节有空位
                for (int bit = 0; bit < 8; bit++) {
                    if (!(map_buf[j] & (1 << bit))) {
                        map_buf[j] |= (1 << bit); // 占用
                        bios_sd_write(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.inode_map_offset + i);
                        return i * SECTOR_SIZE * 8 + j * 8 + bit;
                    }
                }
            }
        }
    }
    return -1; // 满了
}

// 分配空闲 block
uint32_t alloc_block() {
    uint8_t map_buf[SECTOR_SIZE];
    if (!sb_valid) {
        read_sb(&sb);
        sb_valid = 1;
    }
    for (int i = 0; i < BLOCK_MAP_SECTORS; i++) {
        bios_sd_read(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.block_map_offset + i);
        for (int j = 0; j < SECTOR_SIZE; j++) {
            if (map_buf[j] != 0xff) { // 该字节有空位
                for (int bit = 0; bit < 8; bit++) {
                    if (!(map_buf[j] & (1 << bit))) {
                        map_buf[j] |= (1 << bit); // 占用
                        bios_sd_write(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.block_map_offset + i);
                        return i * SECTOR_SIZE * 8 + j * 8 + bit;
                    }
                }
            }
        }
    }
    return -1; // 满了
}

// 从inode map 中释放指定的 inode
void free_inode(uint32_t inode_num) {
    uint8_t map_buf[SECTOR_SIZE];
    read_sb(&sb);
    
    uint32_t sector_idx = inode_num / (SECTOR_SIZE * 8);
    uint32_t byte_idx = (inode_num % (SECTOR_SIZE * 8)) / 8;
    uint32_t bit_idx = inode_num % 8;

    bios_sd_read(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.inode_map_offset + sector_idx);
    map_buf[byte_idx] &= ~(1 << bit_idx); // 将对应位置 0
    bios_sd_write(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.inode_map_offset + sector_idx);
}

// 释放指定的 block
void free_block(uint32_t block_num) {
    uint8_t map_buf[SECTOR_SIZE];
    read_sb(&sb);

    uint32_t sector_idx = block_num / (SECTOR_SIZE * 8);
    uint32_t byte_idx = (block_num % (SECTOR_SIZE * 8)) / 8;
    uint32_t bit_idx = block_num % 8;

    bios_sd_read(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.block_map_offset + sector_idx);
    map_buf[byte_idx] &= ~(1 << bit_idx); // 将对应位置 0
    bios_sd_write(kva2pa((unsigned long)map_buf), 1, sb.start_sector + sb.block_map_offset + sector_idx);
}

// 根据逻辑块号查找物理块号。alloc=1时，若路径上的索引块或数据块不存在则分配
// 分配失败时返回-1，未分配且不需要分配时返回0（文件空洞）
uint32_t get_physical_block_id(inode_t *node, uint32_t inode_num, uint32_t logical_id, int alloc) {
    uint32_t *block_buf = (uint32_t*)TEMP_BUF;
    // 查询直接索引
    if (logical_id < DIRECT_BLOCK_NUM) {
        // .. 和 . 可能指向根目录，此时0是指根目录而不是为分配
        if (node->direct_ptr[logical_id] == 0 && alloc) {
            node->direct_ptr[logical_id] = alloc_block();
            if(node->direct_ptr[logical_id] == -1) return -1;
            write_inode(inode_num, node);
        }
        return node->direct_ptr[logical_id];
    }

    // 一级间接索引
    logical_id -= DIRECT_BLOCK_NUM; 
    if (logical_id < INDIRECT_BLOCK_NUM) {
        if (node->indirect_ptr == 0) { // 还未分配间接索引块
            if (!alloc) return 0;
            node->indirect_ptr = alloc_block();
            if(node->indirect_ptr == -1) return -1;
            memset(block_buf, 0, BLOCK_SIZE);
            write_block(node->indirect_ptr, block_buf);
            write_inode(inode_num, node);
        }
        // 读出索引块内容 / 清零缓冲区
        read_block(node->indirect_ptr, block_buf);
        if (block_buf[logical_id] == 0 && alloc) {
            block_buf[logical_id] = alloc_block();
            if(block_buf[logical_id] == -1) return -1;
            write_block(node->indirect_ptr, block_buf);
        }
        return block_buf[logical_id];
    }

    // 二级间接索引
    logical_id -= INDIRECT_BLOCK_NUM;
    if (logical_id < DOUBLE_INDIRECT_BLOCK_NUM) {
        // 一级索引块中的一个直接索引指向一个二级索引块，代表INDIRECT_BLOCK_NUM个物理块
        uint32_t first_idx = logical_id / INDIRECT_BLOCK_NUM;  // 在一级索引块中的位置
        uint32_t second_idx = logical_id % INDIRECT_BLOCK_NUM; // 在二级索引块中的位置

        // 处理一级索引块
        if (node->double_indirect_ptr == 0) {
            if (!alloc) return 0;
            node->double_indirect_ptr = alloc_block();
            if(node->double_indirect_ptr == -1) return -1;
            memset(block_buf, 0, BLOCK_SIZE);
            write_block(node->double_indirect_ptr, block_buf);
            write_inode(inode_num, node);
        }
        
        // 读取一级索引块，找到对应的二级索引块
        read_block(node->double_indirect_ptr, block_buf);
        if (block_buf[first_idx] == 0) {
            if (!alloc) return 0;
            uint32_t new_level2 = alloc_block();
            if(new_level2 == -1) return -1;
            uint32_t *temp_buf = (uint32_t*)(TEMP_BUF + BLOCK_SIZE);
            memset(temp_buf, 0, INDIRECT_BLOCK_NUM * sizeof(block_idx_t));
            write_block(new_level2, temp_buf);
            block_buf[first_idx] = new_level2; // 指向二级索引块
            write_block(node->double_indirect_ptr, block_buf);
        }

        uint32_t level2_block = block_buf[first_idx];
        read_block(level2_block, block_buf);
        if (block_buf[second_idx] == 0 && alloc) {
            block_buf[second_idx] = alloc_block();
            if(block_buf[second_idx] == -1) return -1;
            write_block(level2_block, block_buf);
        }
        return block_buf[second_idx];
    }

    return -1; // 超过支持的最大范围
}

// 向目录块添加 dentry，并更新 传入的父目录 inode 信息
void append_dentry(inode_t* dir_inode, uint32_t dir_inode_num, dentry_t dentry) {
    uint8_t *block_buf = (uint8_t*)TEMP_BUF;

    uint32_t num_blocks = (dir_inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (uint32_t i = 0; i < num_blocks; i++) {
        uint32_t p_block = get_physical_block_id(dir_inode, dir_inode_num, i, 0); //
        if (p_block == -1 || p_block == 0) continue; 

        read_block(p_block, block_buf); 
        dentry_t *dentries = (dentry_t *)block_buf; 

        for (int j = 0; j < BLOCK_SIZE / sizeof(dentry_t); j++) {
            if (dentries[j].name[0] == '\0') {
                dentries[j] = dentry;
                write_block(p_block, block_buf); 
                return;
            }
        }
    }

    // 如果现有块都满了，分配一个新的逻辑块
    // num_blocks 即为新逻辑块的索引
    uint32_t new_p_block = get_physical_block_id(dir_inode, dir_inode_num, num_blocks, 1);
    if (new_p_block == -1 || new_p_block == 0) {
        printk("[FS] Error: Directory is full or disk out of space!\n");
        return;
    }
    memset(block_buf, 0, BLOCK_SIZE); // 清空新索引块
    dentry_t *new_dentries = (dentry_t *)block_buf;
    new_dentries[0] = dentry;
    write_block(new_p_block, block_buf); // 写回索引块

    // 更新目录 size，写回和其他更新由调用者完成
    dir_inode->size += BLOCK_SIZE; 
    
}

// 从p_inode的目录项中删除t_inode的记录，把对应的 dentry.name[0] 设为 '\0'
void remove_dentry(inode_t* p_inode, uint32_t p_inode_num, char *name){

    uint8_t *block_buf = (uint8_t*)TEMP_BUF;
    uint32_t num_blocks = (p_inode->size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    // 遍历所有逻辑块并清除有关t_inode_num的记录
    for (uint32_t i = 0; i < num_blocks; i++) { 
        uint32_t p_block = get_physical_block_id(p_inode, p_inode_num, i, 0); //
        if (p_block == -1 || p_block == 0) continue;

        read_block(p_block, block_buf); 
        dentry_t *dentries = (dentry_t *)block_buf; 

        for (int j = 0; j < BLOCK_SIZE / sizeof(dentry_t); j++) {
            if (dentries[j].name[0] != '\0' && strcmp(dentries[j].name, name) == 0) {
                dentries[j].name[0] = '\0'; // 标记为空闲
                write_block(p_block, block_buf); 
                return;
            }
        }
    }
    printk("[FS] Error: Dentry for %s not found in parent!\n", name);
}

// 给定目录的inode项idx，查找目录下查找 指定名称文件或文件夹 的inode
uint32_t find_inode_by_name(uint32_t dir_inode_num, char *name) {
    inode_t dir_node;
    get_inode(dir_inode_num, &dir_node);
    if (dir_node.type != FD_DIR) return -1; // 目标不是目录

    uint8_t *block_buf = (uint8_t *)TEMP_BUF;
    uint32_t num_blocks = (dir_node.size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (uint32_t i = 0; i < num_blocks; i++) {
        uint32_t p_block = get_physical_block_id(&dir_node, dir_inode_num, i, 0);
        if (p_block == -1 || p_block == 0) continue;
        read_block(p_block, block_buf); 
        dentry_t *dentries = (dentry_t *)block_buf;
        for (int j = 0; j < BLOCK_SIZE / sizeof(dentry_t); j++) {
            if (dentries[j].name[0] != '\0' && strcmp(dentries[j].name, name) == 0) {
                return dentries[j].inode_num;
            }
        }
    }
    return -1; // 未找到
}

// 路径解析函数，返回路径所指的 inode
uint32_t find_inode_by_path(char *path) {
    // 默认使用当前进程工作目录的相对路径
    uint32_t curr_inode = current_running->dir_inode;
    
    // 识别绝对路径
    if (path[0] == '/') {
        curr_inode = ROOT_INODE_NUM;
        path++; // 跳过开头的 /
    }

    char temp_path[MAX_PATH_LEN];
    strcpy(temp_path, path);
    
    // 路径分割利用 '/' 分割字符串
    char *name = temp_path;
    while (name && *name != '\0') {
        char *next = strchr(name, '/');
        if (next){
            *next = '\0'; 
            curr_inode = find_inode_by_name(curr_inode, name);
            if (curr_inode == -1) return -1;
            name = next + 1;
        }else{
            curr_inode = find_inode_by_name(curr_inode, name);
            break; // 到达最后一级
        }
    }
    return curr_inode;
}

int alloc_fd(){
    for (int fd = 0; fd < NUM_FDESCS; fd++) {
        if (fdesc_array[fd].used == 0) {
            return fd;
        }
    }
    return -1;
}