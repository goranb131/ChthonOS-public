#ifndef _VFS_H
#define _VFS_H

#include <stdint.h>
#include <stddef.h>
#include "message.h"

struct dirent {
    uint64_t inode;
    char name[256];
};

#define VFS_MAX_PATH 256
#define MAX_MOUNTS 16  

typedef int64_t ssize_t;


struct vfs_inode;
struct vfs_file;
struct vfs_super_block;
struct filesystem_type;


struct vfs_file_operations {
    int (*open)(struct vfs_inode *inode, struct vfs_file *file);
    ssize_t (*read)(struct vfs_file *file, char *buf, size_t count);
    ssize_t (*write)(struct vfs_file *file, const void *buf, size_t count);
    int (*close)(struct vfs_file *file);
    int (*unlink)(const char *path);
};


struct vfs_super_operations {
    void (*write_super)(struct vfs_super_block *sb);
    int (*sync_fs)(struct vfs_super_block *sb);
};


struct vfs_inode {
    uint64_t i_no;           
    uint32_t i_mode;         
    uint32_t i_size;         
    struct vfs_super_block *i_sb;  
    struct vfs_file_operations *i_fops;  
};


struct vfs_file {
    struct vfs_inode *inode;  
    uint64_t pos;            
    uint32_t flags;          
    struct vfs_file_operations *f_ops;  
    char f_path[VFS_MAX_PATH];
    void *private_data;
    size_t f_pos;  
};


struct vfs_super_block {
    uint32_t s_magic;        
    const char *s_type;      
    struct vfs_super_operations *s_ops;  
    void *s_fs_info;         
};


struct filesystem_type {
    const char *name;        
    struct vfs_super_block* (*mount)(void);
    struct vfs_file* (*open)(const char *path);
    struct vfs_file *(*create)(const char *path);
    int (*read_dir)(const char *path, struct dirent *dirents, int max_entries);
    int (*unlink)(const char *path);
    int (*mkdir)(const char *path);
    int (*remove_recursive)(const char *path);
};


void vfs_init(void);
int register_filesystem(struct filesystem_type *fs);
int vfs_mount(const char *path, struct filesystem_type *fs);
int vfs_open(const char *path);
ssize_t vfs_read(int fd, void *buf, size_t count);
int vfs_write(int fd, const void *buf, size_t count);

int handle_read_dir_message(struct Message *msg);


static struct mount* find_mount(const char *path);


int resolve_path(const char *path, char *resolved);


int vfs_create(const char *path);  
int vfs_close(int fd);
int vfs_unlink(const char *path);


int vfs_mkdir(const char *path);


int vfs_remove_recursive(const char *path);
int vfs_read_dir(const char* path, struct dirent* dirents, int max_entries);
void vfs_normalize_path(char* path);

#endif