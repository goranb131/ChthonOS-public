#ifndef ABYSSFS_H
#define ABYSSFS_H

#include "vfs.h"
#include <stdint.h>

#define ABYSSFS_MAGIC 0xABCD1234

struct abyssfs_super_block {
    uint32_t magic;          
    uint32_t block_size;     
    uint32_t total_blocks;   
    uint32_t inode_blocks;   
    uint32_t data_blocks;    
    uint32_t free_blocks;    
    uint32_t first_data_block; 
    uint32_t num_inodes;      
};


struct abyssfs_inode {
    uint32_t mode;          
    uint32_t size;          
    uint32_t blocks;        
    uint32_t direct[12];    
    uint32_t indirect;      
};


struct __attribute__((packed)) abyssfs_dir_entry {
    uint32_t inode;
    uint8_t name_len;
    uint8_t rec_len;
    char name[];  
};


#define ABYSSFS_DIR_ENTRY_FIXED_SIZE (sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t))


void abyssfs_init(void);
struct filesystem_type* abyssfs_get_fs_type(void);
void test_inode_alloc(void);
struct vfs_file* abyssfs_create(const char *path);
void abyssfs_run_test(void);
void test_abyssfs(void);

extern struct vfs_file_operations abyssfs_fops;


extern struct filesystem_type abyssfs_fs_type;

#define INODE_BLOCKS 8  
#define INODES_PER_BLOCK (BLOCK_SIZE / sizeof(struct abyssfs_inode))
#define NUM_BLOCKS 64  

#endif 