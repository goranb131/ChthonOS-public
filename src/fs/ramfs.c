#include "ramfs.h"
#include "string.h"
#include "uart.h"
#include "vfs.h"
#include "kmalloc.h"
#include <stdint.h>

#define RAMFS_MAGIC 0x52414D46  
#define MAX_FILES  16
#define MAX_PATH   32
#define MAX_CONTENT 4096

struct ramfs_file {
    char path[MAX_PATH];
    char content[MAX_CONTENT];
    size_t size;
    int used;
};

static struct ramfs_file files[MAX_FILES];


static struct ramfs_file ramfs_root = {
    .path = "/",
    .used = 1
};


static ssize_t ramfs_read(struct vfs_file *file, char *buf, size_t count);

static ssize_t ramfs_write(struct vfs_file *file, const void *buf, size_t count);

static int ramfs_unlink(const char *path);

static int ramfs_mkdir(const char *path);

struct vfs_file_operations ramfs_fops = {
    .read = ramfs_read,
    .write = ramfs_write,
    .open = NULL,
    .close = NULL
};


static ssize_t ramfs_read(struct vfs_file *file, char *buf, size_t count) {
    struct ramfs_file *rf = file->private_data;
    
    uart_puts("RAMFS: Reading at position ");
    uart_hex(file->f_pos);
    uart_puts(" size is ");
    uart_hex(rf->size);
    uart_puts("\n");
    
    
    if (file->f_pos >= rf->size) {
        uart_puts("RAMFS: EOF reached\n");
        return 0;  
    }
    
    
    if (file->f_pos + count > rf->size) {
        count = rf->size - file->f_pos;
        uart_puts("RAMFS: Limiting read to ");
        uart_hex(count);
        uart_puts(" bytes\n");
    }
    
    
    memcpy(buf, rf->content + file->f_pos, count);
    file->f_pos += count;
    
    uart_puts("RAMFS: Read ");
    uart_hex(count);
    uart_puts(" bytes\n");
    
    return count;
}


static ssize_t ramfs_write(struct vfs_file *file, const void *buf, size_t count) {
    struct ramfs_file *rf = file->private_data;
    
    uart_puts("RAMFS: Writing ");
    uart_hex(count);
    uart_puts(" bytes at position ");
    uart_hex(file->f_pos);
    uart_puts(" current size is ");
    uart_hex(rf->size);
    uart_puts("\n");
    
    
    if (file->f_pos + count > MAX_CONTENT) {
        uart_puts("RAMFS: Would overflow, limiting to ");
        uart_hex(MAX_CONTENT - file->f_pos);
        uart_puts(" bytes\n");
        count = MAX_CONTENT - file->f_pos;
    }
    
    if (count > 0) {
        memcpy(rf->content + file->f_pos, buf, count);
        file->f_pos += count;
        if (file->f_pos > rf->size) {
            uart_puts("RAMFS: Updating size from ");
            uart_hex(rf->size);
            uart_puts(" to ");
            uart_hex(file->f_pos);
            uart_puts("\n");
            rf->size = file->f_pos;
        }
        uart_puts("RAMFS: Write successful\n");
    }
    
    return count;
}

static struct vfs_file *ramfs_open(const char *path) {
    uart_puts("RAMFS: Opening file: ");
    uart_puts(path);
    uart_puts("\n");
    
    
    struct ramfs_file *rf = NULL;
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used && strcmp(files[i].path, path) == 0) {
            rf = &files[i];
            break;
        }
    }
    
    if (!rf) {
        return NULL;
    }
    
    
    struct vfs_file *file = kalloc(sizeof(struct vfs_file));
    if (!file) {
        return NULL;
    }
    
    file->f_ops = &ramfs_fops;
    file->private_data = rf;
    file->f_pos = 0;
    strncpy(file->f_path, path, VFS_MAX_PATH - 1);
    file->f_path[VFS_MAX_PATH - 1] = '\0';
    
    return file;
}

void ramfs_init(void) {
    uart_puts("FORCE DEBUG - Initializing RAMFS\n");
    
    memset(files, 0, sizeof(files));
    
    uart_puts("FORCE DEBUG - All files marked unused\n");
}

int ramfs_create_file(const char *path, const char *content)
{
    uart_puts("FORCE DEBUG - Creating file: ");
    uart_puts(path);
    uart_puts("\n");
    
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].size = 0;     
            
            int p = 0;
            while (path[p] && p < (MAX_PATH-1)) {
                files[i].path[p] = path[p];
                p++;
            }
            files[i].path[p] = '\0';

            
            int c = 0;
            while (content[c] && c < (MAX_CONTENT-1)) {
                files[i].content[c] = content[c];
                c++;
            }
            files[i].content[c] = '\0';
            files[i].size = c;  

            return 0; 
        }
    }
    return -1; 
}

int ramfs_read_file(const char *path, char *out, int max_len)
{
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            
            int match = 1;
            for (int j = 0; j < MAX_PATH; j++) {
                if (files[i].path[j] != path[j]) {
                    match = 0;
                    break;
                }
                if (path[j] == '\0') {
                    break;
                }
            }
            if (match) {
                
                int c;
                for (c = 0; c < max_len; c++) {
                    out[c] = files[i].content[c];
                    if (!files[i].content[c]) break;
                }
                return c; 
            }
        }
    }
    return -1; 
}


static struct vfs_super_block* ramfs_mount(void) {
    uart_puts("RAMFS: Mounting filesystem\n");
    
    struct vfs_super_block *sb = kalloc(sizeof(struct vfs_super_block));
    if (!sb) {
        uart_puts("RAMFS: Failed to allocate superblock\n");
        return NULL;
    }
    
    sb->s_magic = RAMFS_MAGIC;  
    sb->s_type = "ramfs";
    sb->s_fs_info = NULL;  
    
    return sb;
}


static struct vfs_file *ramfs_create(const char *path);
static struct vfs_super_block* ramfs_mount(void);
static int ramfs_read_dir(const char* path, struct dirent* dirents, int max_entries);


struct filesystem_type ramfs_fs_type = {
    .name = "ramfs",
    .mount = ramfs_mount,
    .open = ramfs_open,
    .create = ramfs_create,
    .unlink = ramfs_unlink,
    .read_dir = ramfs_read_dir,
    .mkdir = ramfs_mkdir,
    .remove_recursive = NULL  
};


struct ramfs_file* ramfs_find_dir(const char *path);  


static int is_file_in_dir(const char* file_path, const char* dir_path);
static const char* get_basename(const char* path);


char* strrchr(const char* str, int c);


static int ramfs_read_dir(const char* path, struct dirent* dirents, int max_entries) {
    uart_puts("FORCE DEBUG - RAMFS: Reading directory '");
    uart_puts(path);
    uart_puts("'\n");
    
    int entry_count = 0;
    
    uart_puts("FORCE DEBUG - Files array state:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            uart_puts("File ");
            uart_hex(i);
            uart_puts(": ");
            uart_puts(files[i].path);
            uart_puts("\n");
        }
    }
    
    if (entry_count < max_entries) {
        uart_puts("FORCE DEBUG - Adding .\n");
        dirents[entry_count].inode = 1;
        strcpy(dirents[entry_count].name, ".");
        entry_count++;
    }
    
    if (entry_count < max_entries) {
        uart_puts("FORCE DEBUG - Adding ..\n");
        dirents[entry_count].inode = 1;
        strcpy(dirents[entry_count].name, "..");
        entry_count++;
    }
    
    uart_puts("FORCE DEBUG - Checking files array\n");
    for (int i = 0; i < MAX_FILES && entry_count < max_entries; i++) {
        if (files[i].used) {
            uart_puts("FORCE DEBUG - Found used file: '");
            uart_puts(files[i].path);
            uart_puts("'\n");
            
            if (is_file_in_dir(files[i].path, path)) {
                uart_puts("FORCE DEBUG - File matches directory!\n");
                dirents[entry_count].inode = i + 2;
                strcpy(dirents[entry_count].name, get_basename(files[i].path));
                entry_count++;
            }
        }
    }
    
    uart_puts("FORCE DEBUG - Total entries: ");
    uart_hex(entry_count);
    uart_puts("\n");
    
    return entry_count;
}


static int is_file_in_dir(const char* file_path, const char* dir_path) {
    size_t dir_len = strlen(dir_path);
    
    if (strncmp(file_path, dir_path, dir_len) == 0) {
        if (strcmp(dir_path, "/tmp") == 0) {
            if (file_path[dir_len] != '/') return 0;
            
            return !strrchr(file_path + dir_len + 1, '/');
        }
        
        char *last_slash = strrchr(file_path, '/');
        if (!last_slash) return 0;
        
        return (last_slash - file_path) == dir_len;
    }
    
    return 0;
}

static const char* get_basename(const char* path) {
    const char* last_slash = strrchr(path, '/');
    return last_slash ? last_slash + 1 : path;
}


struct ramfs_file* ramfs_find_dir(const char *path) {
    
    return &ramfs_root;
}


static struct vfs_file *ramfs_create(const char *path) {
    uart_puts("RAMFS: Creating file: ");
    uart_puts(path);
    uart_puts("\n");

    
    char full_path[MAX_PATH];
    if (strncmp(path, "/tmp/", 5) != 0) {
        strcpy(full_path, "/tmp/");
        strcat(full_path, path);
    } else {
        strcpy(full_path, path);
    }

    
    int i;
    for (i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            break;
        }
    }

    if (i == MAX_FILES) {
        uart_puts("RAMFS: No free file slots\n");
        return NULL;
    }

    
    files[i].used = 1;
    strcpy(files[i].path, full_path);
    files[i].size = 0;
    files[i].content[0] = '\0';

    
    struct vfs_file *file = kalloc(sizeof(struct vfs_file));
    if (!file) {
        files[i].used = 0;
        return NULL;
    }

    file->f_ops = &ramfs_fops;
    file->private_data = &files[i];
    file->f_pos = 0;
    
    return file;
}

static int ramfs_unlink(const char *path) {
    uart_puts("RAMFS: Unlinking file: ");
    uart_puts(path);
    uart_puts("\n");
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (files[i].used) {
            uart_puts("RAMFS: Checking file: ");
            uart_puts(files[i].path);
            uart_puts("\n");
            
            if (strcmp(files[i].path, path) == 0) {
                uart_puts("RAMFS: Found file, marking unused\n");
                files[i].used = 0;  
                files[i].size = 0;  
                return 0;
            }
        }
    }
    
    uart_puts("RAMFS: File not found\n");
    return -1;  
}

static int ramfs_mkdir(const char *path) {
    uart_puts("RAMFS: Creating directory: ");
    uart_puts(path);
    uart_puts("\n");
    
    for (int i = 0; i < MAX_FILES; i++) {
        if (!files[i].used) {
            files[i].used = 1;
            files[i].size = 0;
            strcpy(files[i].path, path);
            return 0;
        }
    }
    
    return -1;  
}
