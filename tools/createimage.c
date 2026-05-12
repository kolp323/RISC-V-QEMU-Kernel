#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."

#define SECTOR_SIZE 512
#define BOOT_LOADER_SIG_OFFSET 0x1fe
#define OS_SIZE_LOC (BOOT_LOADER_SIG_OFFSET - 2) // 0x1fc
#define TASK_NUM_LOC (OS_SIZE_LOC - 2)  // 0x1fa
#define TASKINFO_OFFSET_LOC (TASK_NUM_LOC - 4)  // 0x1f6
#define BATCH_OFFSET_LOC (TASKINFO_OFFSET_LOC - 4)  // 0x1f2
#define SWAP_OFFSET_LOC (BATCH_OFFSET_LOC - 4)  // 0x1ed

#define BOOT_LOADER_SIG_1 0x55
#define BOOT_LOADER_SIG_2 0xaa

#define NBYTES2SEC(nbytes) (((nbytes) / SECTOR_SIZE) + ((nbytes) % SECTOR_SIZE != 0))

// // ! t3: 假设每个APP占15个扇区-----------------------------
// #define FIXED_APP_SECS 15
// #define FIXED_APP_SIZE (FIXED_APP_SECS * SECTOR_SIZE)
// // ! 即 15×512 字节-----------------------------------

/* TODO: [p1-task4] design your own task_info_t */
#define MAX_NAME_LEN     15
// typedef struct {
//     char name[MAX_NAME_LEN + 1];  // task name
//     uint64_t image_offset;   // App 在image中的地址 
//     int size;      // App 大小
// } task_info_t;//! 大小为32字节
typedef struct {
    char name[MAX_NAME_LEN + 1];
    uint64_t image_offset; // 程序在镜像中的起始偏移
    // 关键段信息 (假设每个程序只有一个 LOAD 段)
    uint64_t vaddr;        // Entry Point / Segment Vaddr
    uint64_t memsz;        // p_memsz
    uint64_t filesz;       // p_filesz
} task_info_t;


#define TASK_MAXNUM 32
static task_info_t taskinfo[TASK_MAXNUM];

// TODO: [p1-task5]
typedef struct {
    uint64_t r_ptr; // 读指针
    uint64_t w_ptr; // 写指针
    int ren; // 读使能
    int wen; // 写使能
} pipe_info_t; // !大小为 24 Bytes
#define PIPE_MAXNUM 2
#define PIPE_SIZE 0x100
#define PIPE_LOC 0x52101000
static pipe_info_t pipeinfo[PIPE_MAXNUM];

// TODO: [p4-task3]
// TODO: 同步修改 os/swap.h
#define SWAP_SIZE (64 * 1024 * 1024) // 手动在镜像内padding了点swap，64MB = 16K页 

/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr *ehdr, FILE *fp);
static void read_phdr(Elf64_Phdr *phdr, FILE *fp, int ph, Elf64_Ehdr ehdr);
static uint64_t get_entrypoint(Elf64_Ehdr ehdr);
static uint32_t get_filesz(Elf64_Phdr phdr);
static uint32_t get_memsz(Elf64_Phdr phdr);
static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr);
static void write_padding(FILE *img, int *phyaddr, int new_phyaddr);
//! 传入phyaddr， 将taskinfo写在所有程序之后
static void write_img_info(int nbytes_kernel, task_info_t *taskinfo,
                           short tasknum, FILE *img, int task_info_offset); 

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock main) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1); // files即为 bootblock + main + app0 + app1 + ...
    return 0;
}

/* TODO: [p1-task4] assign your task_info_t somewhere in 'create_image' */
static void create_image(int nfiles, char *files[])
{
    int tasknum = nfiles - 2;   // app程序的数量
    int nbytes_kernel = 0;      // kernel的字节数
    int phyaddr = 0;            // 当前写入镜像的字节数
    FILE *fp = NULL, *img = NULL;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;

    /* open the image file */
    img = fopen(IMAGE_FILE, "w");
    assert(img != NULL);

    /* for each input file */
    int taskidx; //* 需要使用taskidx来填充剩下的tasks（定义放到了for循环外）
    for (int fidx = 0; fidx < nfiles; ++fidx) {
        int start_phyaddr = phyaddr;            //* 记录当前程序的起始物理地址
        taskidx = fidx - 2;                     // app程序的索引
        /* open input file */
        fp = fopen(*files, "r");
        assert(fp != NULL);

        /* read ELF header */
        read_ehdr(&ehdr, fp);                  // 读取ELF头
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (int ph = 0; ph < ehdr.e_phnum; ph++) {
            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);
            if (phdr.p_type != PT_LOAD) continue;

            // TODO: [p4-task1] 写入task_info，只记录LOAD段
            if(taskidx >= 0){
                strncpy(taskinfo[taskidx].name, *files, MAX_NAME_LEN+1);
                // 假设只有一个 LOAD 段，直接记录
                taskinfo[taskidx].vaddr = phdr.p_vaddr;
                taskinfo[taskidx].memsz = phdr.p_memsz;
                taskinfo[taskidx].filesz = phdr.p_filesz;
                
            }
            
            /* write segment to the image */
            write_segment(phdr, fp, img, &phyaddr);
            /* update nbytes_kernel */
            if (strcmp(*files, "main") == 0) {
                nbytes_kernel += get_filesz(phdr);
            }
        }

        if(taskidx >= 0){
            strncpy(taskinfo[taskidx].name, *files, 15);
            taskinfo[taskidx].image_offset = start_phyaddr;
        }


        /* write padding bytes */
        /**
         * TODO:
         * 1. [p1-task3] do padding so that the kernel and every app program
         *  occupies the same number of sectors
         * 2. [p1-task4] only padding bootblock is allowed!
         */
        if (strcmp(*files, "bootblock") == 0) {
            write_padding(img, &phyaddr, SECTOR_SIZE);
        }

        fclose(fp);
        files++;
    }
    // 现在的taskidx = tasknum - 1
    // ! 把剩下的taskinfo做0填充
    for (int i = taskidx + 1 ; i < TASK_MAXNUM; i++) {
        taskinfo[i].name[0] = '\0';
    }

    // TODO：[p1-task5] 初始化 pipeinfo，然后在write_img_info写入taskinfo后面
    for (int i = 0; i < PIPE_MAXNUM; i++) {
        // 关闭使能
        pipeinfo[i].wen = 0;
        pipeinfo[i].ren = 0;
        // 设置读写指针
        pipeinfo[i].r_ptr = PIPE_LOC + i * PIPE_SIZE;
        pipeinfo[i].w_ptr = PIPE_LOC + i * PIPE_SIZE;
    }

    //! 传入phyaddr， 将 taskinfo 和 pipeinfo 写在所有程序之后
    write_img_info(nbytes_kernel, taskinfo, tasknum, img, phyaddr);

    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    int ret;

    ret = fread(ehdr, sizeof(*ehdr), 1, fp);
    assert(ret == 1);
    assert(ehdr->e_ident[EI_MAG1] == 'E');
    assert(ehdr->e_ident[EI_MAG2] == 'L');
    assert(ehdr->e_ident[EI_MAG3] == 'F');
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    int ret;

    fseek(fp, ehdr.e_phoff + ph * ehdr.e_phentsize, SEEK_SET);
    ret = fread(phdr, sizeof(*phdr), 1, fp);
    assert(ret == 1);
    if (options.extended == 1) {
        printf("\tsegment %d\n", ph);
        printf("\t\toffset 0x%04lx", phdr->p_offset);
        printf("\t\tvaddr 0x%04lx\n", phdr->p_vaddr);
        printf("\t\tfilesz 0x%04lx", phdr->p_filesz);
        printf("\t\tmemsz 0x%04lx\n", phdr->p_memsz);
    }
}

static uint64_t get_entrypoint(Elf64_Ehdr ehdr)
{
    return ehdr.e_entry;
}

static uint32_t get_filesz(Elf64_Phdr phdr)
{
    return phdr.p_filesz;
}

static uint32_t get_memsz(Elf64_Phdr phdr)
{
    return phdr.p_memsz;
}

static void write_segment(Elf64_Phdr phdr, FILE *fp, FILE *img, int *phyaddr)
{
    if (phdr.p_memsz != 0 && phdr.p_type == PT_LOAD) {
        /* write the segment itself */
        /* NOTE: expansion of .bss should be done by kernel or runtime env! */
        if (options.extended == 1) {
            printf("\t\twriting 0x%04lx bytes\n", phdr.p_filesz);
        }
        fseek(fp, phdr.p_offset, SEEK_SET);
        while (phdr.p_filesz-- > 0) {
            fputc(fgetc(fp), img);
            (*phyaddr)++;
        }
    }
}

static void write_padding(FILE *img, int *phyaddr, int new_phyaddr)
{
    if (options.extended == 1 && *phyaddr < new_phyaddr) {
        printf("\t\twrite 0x%04x bytes for padding\n", new_phyaddr - *phyaddr);
    }

    while (*phyaddr < new_phyaddr) {
        fputc(0, img);
        (*phyaddr)++;
    }
}

static void write_img_info(int nbytes_kernel, task_info_t *taskinfo,
                           short tasknum, FILE * img, int task_info_offset)
{
    // TODO: [p1-task3] & [p1-task4] write image info to some certain places
    // NOTE: os size, infomation about app-info sector(s) ...可以写到固定位置以便bootloader读取
    
    // 1. 写入task_info
    // ! 此时文件指针正好在所有程序末尾，在此写入task_info
    printf("tasknum: %d\n", tasknum);
    fwrite(taskinfo, sizeof(task_info_t), tasknum, img);

    // TODO: 在下一个扇区预留至少一个扇区的空间，给batch sequence
    int haven_offset = task_info_offset + TASK_MAXNUM * sizeof(task_info_t);
    int rest = haven_offset % SECTOR_SIZE;
    unsigned start_block = rest ? (haven_offset / SECTOR_SIZE + 1) : (haven_offset / SECTOR_SIZE); // 扇区对齐
    int batch_offset = start_block * SECTOR_SIZE;   // *批处理序列的起始位置
    int swap_offset = batch_offset + SECTOR_SIZE;   // *swap空间的位置
    // 为批处理序列预留位置
    fseek(img, start_block * SECTOR_SIZE, SEEK_SET);
    write_padding(img, &haven_offset, batch_offset + SECTOR_SIZE);
    // // 为 SWAP 区域预留空间
    // int swap_start = batch_offset + SECTOR_SIZE;
    // write_padding(img, &swap_start, swap_start + SWAP_SIZE);

    // 2. 写入内核扇区数
    // 计算内核扇区数
    short numsecs_kernel = NBYTES2SEC(nbytes_kernel);   
    // 将文件指针定位到OS_SIZE_LOC
    fseek(img, OS_SIZE_LOC, SEEK_SET);
    fwrite(&numsecs_kernel, sizeof(numsecs_kernel), 1, img);

    // 3. 写入内核签名
    short signature = (BOOT_LOADER_SIG_2 << 8) | BOOT_LOADER_SIG_1;
    fseek(img, BOOT_LOADER_SIG_OFFSET, SEEK_SET);
    fwrite(&signature, sizeof(signature), 1, img);

    // 4. 写入tasknum
    fseek(img, TASK_NUM_LOC, SEEK_SET);
    fwrite(&tasknum, sizeof(tasknum), 1, img);

    // 5. 写入taskinfo_offset
    fseek(img, TASKINFO_OFFSET_LOC, SEEK_SET);
    fwrite(&task_info_offset, sizeof(task_info_offset), 1, img);

    // 6. 写入batch_offset
    fseek(img, BATCH_OFFSET_LOC, SEEK_SET);
    fwrite(&batch_offset, sizeof(batch_offset), 1, img);

    // 7. 写入swap_offset
    fseek(img, SWAP_OFFSET_LOC, SEEK_SET);
    fwrite(&swap_offset, sizeof(swap_offset), 1, img);
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
