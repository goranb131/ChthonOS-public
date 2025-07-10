#include "abyssfs.h"
#include "kmalloc.h"
#include "uart.h"
#include "string.h"
#include <stddef.h>
#include "vfs.h"  
#include "process.h"  


extern const char* pwd(void);  
const char* pwd(void);  
static uint8_t* get_block(uint32_t block_num);
static uint32_t alloc_block(void);
static void free_block(uint32_t block_num);
static struct abyssfs_inode* get_inode(uint32_t inode_num);
static uint32_t alloc_inode(void);
static int create_file(const char *name, uint32_t inode_num);
static ssize_t abyssfs_read(struct vfs_file *file, char *buf, size_t count);
static struct vfs_file* abyssfs_open(const char *path);
static int abyssfs_read_dir(const char *path, struct dirent *dirents, int max_entries);
static struct vfs_file *abyssfs_create_file(const char *path);
static ssize_t abyssfs_write(struct vfs_file *file, const void *buf, size_t count);
static size_t write_to_blocks(struct abyssfs_inode *inode, const void *buf, size_t count);
static int abyssfs_unlink(const char *path);
static struct abyssfs_inode* get_inode_by_path(const char *path);
static int abyssfs_mkdir(const char *path);
static int abyssfs_remove_recursive(const char *path);


#define BLOCK_SIZE 4096


static struct {
    struct abyssfs_super_block sb;
    uint8_t *blocks;  
} abyssfs;


static uint64_t block_bitmap = 0;
static int abyssfs_initialized = 0;

#define ABYSSFS_DIR_ENTRY_FIXED_SIZE (sizeof(uint32_t) + sizeof(uint8_t) + sizeof(uint8_t))


static inline uint8_t round_up(uint8_t val, uint8_t align) {
    return (val + align - 1) & ~(align - 1);
}


static struct vfs_super_block* abyssfs_mount(void) {
    struct vfs_super_block* vsb = kalloc(sizeof(struct vfs_super_block));
    if (!vsb) return NULL;

    abyssfs_init();

    vsb->s_magic = ABYSSFS_MAGIC;
    vsb->s_type = "abyssfs";
    vsb->s_fs_info = &abyssfs;

    return vsb;
}


struct vfs_file_operations abyssfs_fops = {
    .read = abyssfs_read,
    .write = abyssfs_write,
    .open = NULL,
    .close = NULL
};


struct filesystem_type abyssfs_fs_type = {
    .name = "abyssfs",
    .mount = abyssfs_mount,
    .open = abyssfs_open,
    .create = abyssfs_create,
    .unlink = abyssfs_unlink,
    .read_dir = abyssfs_read_dir,
    .mkdir = abyssfs_mkdir,
    .remove_recursive = abyssfs_remove_recursive
};


void abyssfs_init(void) {
    
    if (abyssfs_initialized) {
        uart_puts("FORCE DEBUG - AbyssFS already initialized\n");
        return;
    }
    
    size_t total_size = BLOCK_SIZE * 64;
    abyssfs.blocks = kalloc(total_size);
    if (!abyssfs.blocks) {
        uart_puts("Failed to allocate AbyssFS blocks\n");
        return;
    }

    memset(abyssfs.blocks, 0, total_size);
    
    abyssfs.sb.magic = ABYSSFS_MAGIC;
    abyssfs.sb.block_size = BLOCK_SIZE;
    abyssfs.sb.total_blocks = 64;  
    abyssfs.sb.inode_blocks = 8;   
    abyssfs.sb.data_blocks = abyssfs.sb.total_blocks - abyssfs.sb.inode_blocks - 1;
    abyssfs.sb.free_blocks = abyssfs.sb.data_blocks;
    abyssfs.sb.first_data_block = abyssfs.sb.inode_blocks + 1;
    
    uart_puts("AbyssFS initialized with ");
    uart_putc('0' + abyssfs.sb.total_blocks/10);
    uart_putc('0' + abyssfs.sb.total_blocks%10);
    uart_puts(" blocks (");
    uart_putc('0' + abyssfs.sb.inode_blocks);
    uart_puts(" inode blocks)\n");

    
    for (uint32_t i = 0; i < abyssfs.sb.inode_blocks; i++) {
        struct abyssfs_inode* inode = get_inode(i);
        if (inode) {
            memset(inode, 0, sizeof(struct abyssfs_inode));
        }
    }

    
    block_bitmap = 0;
    for (uint32_t i = 0; i <= abyssfs.sb.inode_blocks; i++) {
        block_bitmap |= (1ULL << i);
    }

    
struct abyssfs_inode *root_inode = get_inode(1);
if (root_inode) {
    
    root_inode->mode = 0x4000;  
    
    root_inode->size = BLOCK_SIZE;  
    
    root_inode->blocks = abyssfs.sb.inode_blocks + 1;
    
    
    struct abyssfs_dir_entry *root_dir = (struct abyssfs_dir_entry *)get_block(root_inode->blocks);
    if (root_dir) {
        
        root_dir->inode = 1;
        root_dir->name_len = 1;
        uint8_t rec = round_up(ABYSSFS_DIR_ENTRY_FIXED_SIZE + 1 + 1, 4);
        root_dir->rec_len = rec;
        strcpy(root_dir->name, ".");
        
        
        struct abyssfs_dir_entry *dotdot = (struct abyssfs_dir_entry *)((char *)root_dir + rec);
        dotdot->inode = 1;
        dotdot->name_len = 2;
        dotdot->rec_len = BLOCK_SIZE - rec;  
        strcpy(dotdot->name, "..");
    }
}

    uint32_t root_dir_block = abyssfs.sb.inode_blocks + 1;
    block_bitmap |= (1ULL << root_dir_block);

    abyssfs_initialized = 1;
    uart_puts("FORCE DEBUG - AbyssFS initialized for first time\n");
}


static void debug_dir_entry(struct abyssfs_dir_entry *dir) {
    uart_puts("Dir entry: inode=");
    uart_putc('0' + dir->inode);
    uart_puts(", name=");
    uart_puts(dir->name);
    uart_puts("\n");
}


static int create_file(const char *name, uint32_t inode_num) {
    /*uart_puts("DEBUG: create_file called with name='");
    uart_puts(name);
    uart_puts("' inode_num=");
    uart_hex(inode_num);
    uart_puts("\n");*/
    
    char path_copy[256];
    strncpy(path_copy, name, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    char *last_slash = strrchr(path_copy, '/');
    const char *filename;
    struct abyssfs_inode *dir_inode;
    
    if (last_slash) {
        *last_slash = '\0';
        filename = last_slash + 1;
        
        if (path_copy[0] == '\0') {
            dir_inode = get_inode(1);  
            //uart_puts("DEBUG: Using root directory inode\n");
        } else {
            dir_inode = get_inode_by_path(path_copy);
            if (!dir_inode) {
                uart_puts("DEBUG: Failed to get directory inode for path: ");
                uart_puts(path_copy);
                uart_puts("\n");
                return -1;
            }
            /*uart_puts("DEBUG: Using directory inode for path: ");
            uart_puts(path_copy);
            uart_puts("\n");*/
        }
    } else {
        filename = name;
        if (strcmp(current_process->cwd, "/") != 0) {
            const char *path = current_process->cwd + 1; 
            dir_inode = get_inode_by_path(path);
            if (!dir_inode) {
                uart_puts("Failed to get directory inode\n");
                return -1;
            }
        } else {
            dir_inode = get_inode(1);  
            //uart_puts("DEBUG: Using root directory inode\n");
        }
    }
    
    
    uint32_t dir_block = dir_inode->blocks;
    if (dir_block == 0) {
        if (dir_inode == get_inode(1)) {
            dir_block = abyssfs.sb.inode_blocks + 1;
        } else {
            uart_puts("Invalid directory block\n");
            return -1;
        }
    }
    
    char *block_start = (char *)get_block(dir_block);
    char *block_end = block_start + BLOCK_SIZE;
    struct abyssfs_dir_entry *entry = (struct abyssfs_dir_entry *)block_start;
    struct abyssfs_dir_entry *prev = NULL;
    
    while ((char *)entry < block_end && entry->rec_len > 0) {
        prev = entry;
        entry = (struct abyssfs_dir_entry *)((char *)entry + entry->rec_len);
    }
    
    if (prev) {
        uint8_t prev_min = round_up(ABYSSFS_DIR_ENTRY_FIXED_SIZE + prev->name_len + 1, 4);
        char *prev_entry_start = (char *)prev;
        prev->rec_len = prev_min;
        entry = (struct abyssfs_dir_entry *)(prev_entry_start + prev_min);
        memset(entry, 0, block_end - (char *)entry);
    } else {
        entry = (struct abyssfs_dir_entry *)block_start;
    }
    
    entry->inode = inode_num;
    entry->name_len = strlen(filename);
    entry->rec_len = round_up(ABYSSFS_DIR_ENTRY_FIXED_SIZE + entry->name_len + 1, 4);
    memcpy(entry->name, filename, entry->name_len);
    entry->name[entry->name_len] = '\0';
    
    return 0;
}


static ssize_t abyssfs_read(struct vfs_file *file, char *buf, size_t count) {
    uint32_t inode_num = (uint32_t)(uintptr_t)file->private_data;
    
    struct abyssfs_inode *inode = get_inode(inode_num);
    if (!inode) {
        return -1;
    }
    
    if (file->f_pos >= inode->size) {
        return 0;  
    }
    
    if (file->f_pos + count > inode->size) {
        count = inode->size - file->f_pos;
    }
    
    memcpy(buf, abyssfs.blocks + (inode->blocks * BLOCK_SIZE) + file->f_pos, count);
    file->f_pos += count;
    
    return count;
}


static ssize_t abyssfs_write(struct vfs_file *file, const void *buf, size_t count) {
    uint32_t inode_num = (uint32_t)(uintptr_t)file->private_data;
    
    
    struct abyssfs_inode *inode = get_inode(inode_num);
    if (!inode) {
        return -1;
    }
    
    size_t written = write_to_blocks(inode, buf, count);
    
    if (written > 0) {
        inode->size += written;
    }
    
    return written;
}


static uint8_t* get_block(uint32_t block_num) {
    if (block_num >= abyssfs.sb.total_blocks) {
        return NULL;
    }
    return abyssfs.blocks + (block_num * BLOCK_SIZE);
}

static uint32_t alloc_block(void) {
    
    for (uint32_t i = abyssfs.sb.first_data_block; i < abyssfs.sb.total_blocks; i++) {
        if (!(block_bitmap & (1ULL << i))) {
            block_bitmap |= (1ULL << i);
            abyssfs.sb.free_blocks--;
            return i;
        }
    }
    return 0;  
}

static void free_block(uint32_t block_num) {
    if (block_num < abyssfs.sb.total_blocks) {
        block_bitmap &= ~(1ULL << block_num);
        abyssfs.sb.free_blocks++;
    }
}


static struct abyssfs_inode* get_inode(uint32_t inode_num) {
    if (inode_num >= (abyssfs.sb.inode_blocks * BLOCK_SIZE / sizeof(struct abyssfs_inode))) {
        return NULL;
    }
    
    uint32_t inodes_per_block = BLOCK_SIZE / sizeof(struct abyssfs_inode);
    uint32_t block = 1 + (inode_num / inodes_per_block);  
    uint32_t offset = inode_num % inodes_per_block;
    
    uint8_t* block_ptr = get_block(block);
    if (!block_ptr) return NULL;
    
    return (struct abyssfs_inode*)block_ptr + offset;
}

static uint32_t alloc_inode(void) {
    uint32_t inodes_per_block = BLOCK_SIZE / sizeof(struct abyssfs_inode);
    uint32_t max_inodes = abyssfs.sb.inode_blocks * inodes_per_block;
    
    //uart_puts("Searching for free inode (max: ");
    
    /*if (max_inodes >= 100) {
        uart_putc('0' + (max_inodes/100));
        uart_putc('0' + ((max_inodes/10)%10));
        uart_putc('0' + (max_inodes%10));
    } else if (max_inodes >= 10) {
        uart_putc('0' + (max_inodes/10));
        uart_putc('0' + (max_inodes%10));
    } else {
        uart_putc('0' + max_inodes);
    }
    uart_puts(")\n");*/
    
    
    for (uint32_t i = 1; i < max_inodes; i++) {
        struct abyssfs_inode* inode = get_inode(i);
        if (!inode) {
            uart_puts("Failed to get inode ");
            uart_putc('0' + i);
            uart_puts("\n");
            continue;
        }
        if (inode->mode == 0) {
            memset(inode, 0, sizeof(struct abyssfs_inode));
            inode->mode = 0x1FF;
            return i;
        }
    }
    uart_puts("No free inodes found\n");
    return 0;
}


void test_inode_alloc(void) {
    uint32_t inode = alloc_inode();
    uart_puts("Allocated inode: ");
    uart_putc('0' + inode);
    uart_puts("\n");
}


void test_abyssfs(void) {
    uart_puts("\n=== Testing AbyssFS ===\n");
    
    uart_puts("Testing inode allocation:\n");
    uint32_t inode_num = alloc_inode();
    uart_puts("Allocated inode: ");
    uart_putc('0' + inode_num);
    uart_puts("\n");
    
    struct abyssfs_inode *inode = get_inode(inode_num);
    if (inode) {
        inode->mode = 0x1FF;  
        inode->size = 0;      
        inode->blocks = 0;    
        uart_puts("Inode set up successfully\n");
    } else {
        uart_puts("Failed to get inode\n");
        return;
    }
    
    uart_puts("About to create file with inode: ");
    uart_putc('0' + inode_num);
    uart_puts("\n");
    
    const char *test_file = "hello.txt";
    if (create_file(test_file, inode_num) == 0) {
        uart_puts("Created file: /");
        uart_puts(test_file);
        uart_puts("\n");
    } else {
        uart_puts("Failed to create file\n");
    }
}


struct filesystem_type* abyssfs_get_fs_type(void) {
    return &abyssfs_fs_type;
}

 struct vfs_file* abyssfs_create(const char *path);  

 struct vfs_file* abyssfs_create(const char *path) {
    
    if (*path == '/') path++;
    
    uint32_t inode_num = alloc_inode();
    if (inode_num == 0) {
        return NULL;
    }
      
    struct abyssfs_inode *inode = get_inode(inode_num);
    if (!inode) {
        return NULL;
    }
    
    if (create_file(path, inode_num) < 0) {
        
        return NULL;
    }
    
    struct vfs_file *file = kalloc(sizeof(struct vfs_file));
    if (!file) {
        
        return NULL;
    }
    
    file->f_pos = 0;
    file->f_ops = &abyssfs_fops;
    file->private_data = (void*)(uintptr_t)inode_num;
    
    return file;
}


static struct vfs_file* abyssfs_open(const char *path) {
    
    
    struct vfs_file *file = kalloc(sizeof(struct vfs_file));
    if (!file) return NULL;
    
    
    file->f_ops = &abyssfs_fops;  
    file->private_data = NULL;
    strncpy(file->f_path, path, VFS_MAX_PATH - 1);
    file->f_path[VFS_MAX_PATH - 1] = '\0';
    
    
    return file;
}


void abyssfs_run_test(void) {
    test_abyssfs();
}


static int abyssfs_read_dir(const char *path, struct dirent *dirents, int max_entries) {
    //uart_puts("FORCE DEBUG - AbyssFS reading dir: ");
    //uart_puts(path);
    //uart_puts("\n");
    
    size_t entry_count = 0;
    
    if (*path == '/') path++;
    
    uint32_t dir_block;
    if (*path == '\0') {
        dir_block = abyssfs.sb.inode_blocks + 1;
        //uart_puts("DEBUG: Reading root directory, block=");
        //uart_hex(dir_block);
        //uart_puts("\n");
    } else {
        struct abyssfs_inode *dir_inode = get_inode_by_path(path);
        if (!dir_inode) {
            //uart_puts("DEBUG: Directory inode not found\n");
            return -1;
        }
        dir_block = dir_inode->blocks;
        //uart_puts("DEBUG: Reading subdirectory, block=");
        //uart_hex(dir_block);
        //uart_puts("\n");
    }
    
    struct abyssfs_dir_entry *dir = (struct abyssfs_dir_entry *)get_block(dir_block);
    if (!dir) {
        uart_puts("DEBUG: Failed to get directory block\n");
        return -1;
    }
    
    //uart_puts("DEBUG: Starting directory scan\n");
    while (dir && dir->rec_len > 0 && entry_count < max_entries) {
        //uart_puts("DEBUG: Found entry '");
        //uart_puts(dir->name);
        //uart_puts("' inode=");
        //uart_hex(dir->inode);
        //uart_puts(" rec_len=");
        //uart_hex(dir->rec_len);
        //uart_puts("\n");
        
        dirents[entry_count].inode = dir->inode;
        strcpy(dirents[entry_count].name, dir->name);
        entry_count++;
        
        dir = (struct abyssfs_dir_entry *)((char *)dir + dir->rec_len);
    }
    
    //uart_puts("DEBUG: Total entries found: ");
    //uart_hex(entry_count);
    //uart_puts("\n");
    
    return entry_count;
}


static int is_file_in_dir(const char* file_path, const char* dir_path) {
    if (*dir_path == '/') dir_path++;
    if (*file_path == '/') file_path++;
    
    while (*dir_path && *dir_path == *file_path) {
        dir_path++;
        file_path++;
    }
    
    return (*dir_path == '\0' && (*file_path == '/' || *file_path == '\0'));
}

static const char* get_basename(const char* path) {
    const char* last_slash = strrchr(path, '/');
    return last_slash ? last_slash + 1 : path;
}

static int allocate_blocks(void) {
    //uart_puts("FORCE DEBUG - Allocating blocks for AbyssFS\n");
    
    if (abyssfs.blocks) {
        //uart_puts("FORCE DEBUG - Blocks already allocated\n");
        return 0;
    }

    abyssfs.blocks = kalloc(BLOCK_SIZE * NUM_BLOCKS);
    if (!abyssfs.blocks) {
        //uart_puts("FORCE DEBUG - Failed to allocate blocks\n");
        return -1;
    }

    //uart_puts("FORCE DEBUG - Successfully allocated blocks\n");
    return 0;
}


static struct vfs_file *abyssfs_create_file(const char *path) {
    //uart_puts("AbyssFS: Creating file: ");
    //uart_puts(path);
    //uart_puts("\n");

    
    if (*path == '/') path++;

    
    uint32_t inode_num = alloc_inode();
    if (inode_num == 0) {
        uart_puts("AbyssFS: Failed to allocate inode\n");
        return NULL;
    }

    
    if (create_file(path, inode_num) < 0) {
        uart_puts("AbyssFS: Failed to create file\n");
        return NULL;
    }

    
    struct vfs_file *file = kalloc(sizeof(struct vfs_file));
    if (!file) {
        return NULL;
    }

    file->f_ops = &abyssfs_fops;
    file->private_data = (void*)(uintptr_t)inode_num;
    file->f_pos = 0;  
    strncpy(file->f_path, path, VFS_MAX_PATH - 1);
    file->f_path[VFS_MAX_PATH - 1] = '\0';

    return file;
}


static size_t write_to_blocks(struct abyssfs_inode *inode, const void *buf, size_t count) {
    
    if (count > BLOCK_SIZE) {
        count = BLOCK_SIZE;  
    }
    
    
    if (inode->blocks == 0) {
        uint32_t block_num = alloc_block();
        if (block_num == 0) {
            return 0;
        }
        inode->blocks = block_num;
    }
    
    
    memcpy(abyssfs.blocks + (inode->blocks * BLOCK_SIZE), buf, count);
    
    return count;
}


static int abyssfs_unlink(const char *path) {
    
    if (*path == '/') path++;
    
    
    uint32_t root_block = abyssfs.sb.inode_blocks + 1;
    struct abyssfs_dir_entry *dir = (struct abyssfs_dir_entry *)get_block(root_block);
    struct abyssfs_dir_entry *prev = NULL;
    
    //uart_puts("AbyssFS: Looking for entry: '");
    //uart_puts(path);
    //uart_puts("'\n");
    
    
    while (dir && dir->rec_len > 0) {
        //uart_puts("AbyssFS: Checking entry: '");
        //uart_puts(dir->name);
        //uart_puts("'\n");
        
        if (strcmp(dir->name, path) == 0) {
            //uart_puts("AbyssFS: Found entry to remove\n");
            
            
            struct abyssfs_inode *inode = get_inode(dir->inode);
            if (inode) {
                inode->mode = 0;
                if (inode->blocks) {
                    free_block(inode->blocks);
                }
            }
            
            
            struct abyssfs_dir_entry *next = (struct abyssfs_dir_entry *)((char *)dir + dir->rec_len);
            if (next->rec_len > 0) {
                size_t remaining = BLOCK_SIZE - ((char *)next - (char *)get_block(root_block));
                memmove(dir, next, remaining);
            } else {
                
                memset(dir, 0, dir->rec_len);
            }
            
            return 0;
        }
        prev = dir;
        dir = (struct abyssfs_dir_entry *)((char *)dir + dir->rec_len);
    }
    
    uart_puts("AbyssFS: Entry not found\n");
    return -1;
}

static struct abyssfs_inode* get_inode_by_path(const char *path) {
    
    struct abyssfs_inode *inode = get_inode(1);  
    
    
    if (*path == '/') path++;
    
    
    if (*path == '\0') {
        return inode;
    }
    
    char path_copy[256];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    
    char *saveptr;
    char *token = strtok_r(path_copy, "/", &saveptr);
    while (token) {
        
        uint32_t dir_block = inode->blocks;
        if (dir_block == 0) {
            uart_puts("Invalid directory block\n");
            return NULL;
        }
        
        
        struct abyssfs_dir_entry *dir = (struct abyssfs_dir_entry *)get_block(dir_block);
        int found = 0;
        
        while (dir && dir->rec_len > 0) {
            //uart_puts("Checking entry: ");
            //uart_puts(dir->name);
            //uart_puts("\n");
            
            if (strcmp(dir->name, token) == 0) {
                inode = get_inode(dir->inode);
                found = 1;
                break;
            }
            dir = (struct abyssfs_dir_entry *)((char *)dir + dir->rec_len);
        }
        
        if (!found) {
            uart_puts("Path component not found: ");
            uart_puts(token);
            uart_puts("\n");
            return NULL;
        }
        
        token = strtok_r(NULL, "/", &saveptr);
    }
    
    return inode;
}

static int abyssfs_mkdir(const char *path) {
    //uart_puts("AbyssFS: Creating directory: ");
    //uart_puts(path);
    //uart_puts("\n");
    
    char path_copy[VFS_MAX_PATH];
    strcpy(path_copy, path);
    
    char *last_slash = strrchr(path_copy, '/');
    if (!last_slash) {
        //uart_puts("AbyssFS: Creating directory in current directory\n");
        return -1; 
    }
    
    const char *basename = last_slash + 1;
    if (strlen(basename) == 0) {
        uart_puts("AbyssFS: Empty basename\n");
        return -1;
    }
    
    *last_slash = '\0';
    const char *parent_path = path_copy;
    
    if (strlen(parent_path) == 0) {
        parent_path = "/";
    }
    
    //uart_puts("AbyssFS: Parent path: ");
    //uart_puts(parent_path);
    //uart_puts(", basename: ");
    //uart_puts(basename);
    //uart_puts("\n");
    
    struct abyssfs_inode *parent_inode;
    uint32_t parent_inode_num;
    if (strcmp(parent_path, "/") == 0) {
        parent_inode = get_inode(1);  
        parent_inode_num = 1;
    } else {
        const char *lookup_path = parent_path + 1;
        parent_inode = get_inode_by_path(lookup_path);
        if (!parent_inode) {
            uart_puts("AbyssFS: Parent directory not found\n");
            return -1;
        }

        parent_inode_num = 1; // default to root, should be improved
        for (uint32_t i = 1; i <= 512; i++) {
            if (get_inode(i) == parent_inode) {
                parent_inode_num = i;
                break;
            }
        }
    }
    
    uint32_t inode_num = alloc_inode();
    if (inode_num == 0) {
        return -1;
    }
    
    struct abyssfs_inode *inode = get_inode(inode_num);
    if (!inode) {
        return -1;
    }
    
    uint32_t dir_block = alloc_block();
    if (dir_block == 0) {
        
        return -1;
    }
    
    inode->blocks = dir_block;
    inode->size = BLOCK_SIZE;
    inode->mode |= 0x4000;  
    
    struct abyssfs_dir_entry *dir = (struct abyssfs_dir_entry *)get_block(dir_block);
    if (!dir) {
        
        return -1;
    }
    
    dir->inode = inode_num;
    dir->name_len = 1;
    uint8_t rec_len = round_up(ABYSSFS_DIR_ENTRY_FIXED_SIZE + 1 + 1, 4);
    dir->rec_len = rec_len;
    strcpy(dir->name, ".");
    
    struct abyssfs_dir_entry *dotdot = (struct abyssfs_dir_entry *)((char *)dir + rec_len);
    dotdot->inode = parent_inode_num;
    dotdot->name_len = 2;
    dotdot->rec_len = BLOCK_SIZE - rec_len;  
    strcpy(dotdot->name, "..");
    
    uint32_t parent_block = parent_inode->blocks;
    if (parent_block == 0) {
        if (parent_inode == get_inode(1)) {
            parent_block = abyssfs.sb.inode_blocks + 1;
        } else {
            uart_puts("Invalid parent directory block\n");
            return -1;
        }
    }
    
    char *block_start = (char *)get_block(parent_block);
    char *block_end = block_start + BLOCK_SIZE;
    struct abyssfs_dir_entry *entry = (struct abyssfs_dir_entry *)block_start;
    struct abyssfs_dir_entry *prev = NULL;
    
    while ((char *)entry < block_end && entry->rec_len > 0) {
        prev = entry;
        entry = (struct abyssfs_dir_entry *)((char *)entry + entry->rec_len);
    }
    
    if (prev) {
        uint8_t prev_min = round_up(ABYSSFS_DIR_ENTRY_FIXED_SIZE + prev->name_len + 1, 4);
        char *prev_entry_start = (char *)prev;
        prev->rec_len = prev_min;
        entry = (struct abyssfs_dir_entry *)(prev_entry_start + prev_min);
        memset(entry, 0, block_end - (char *)entry);
    } else {
        entry = (struct abyssfs_dir_entry *)block_start;
    }
    
    entry->inode = inode_num;
    entry->name_len = strlen(basename);
    entry->rec_len = round_up(ABYSSFS_DIR_ENTRY_FIXED_SIZE + entry->name_len + 1, 4);
    memcpy(entry->name, basename, entry->name_len);
    entry->name[entry->name_len] = '\0';
    
    return 0;
}

static int abyssfs_remove_recursive(const char *path) {
    
    struct dirent entries[256];  
    int max_entries = 256;
    
    int entry_count = abyssfs_read_dir(path, entries, max_entries);
    if (entry_count > 0) {
        
        for (int i = 0; i < entry_count; i++) {
            
            if (strcmp(entries[i].name, ".") == 0 || 
                strcmp(entries[i].name, "..") == 0) {
                continue;
            }
            
            char full_path[VFS_MAX_PATH];
            strcpy(full_path, path);
            strcat(full_path, "/");
            strcat(full_path, entries[i].name);
            
            
            struct abyssfs_inode *inode = get_inode(entries[i].inode);
            if (inode && (inode->mode & 0x4000)) {
                abyssfs_remove_recursive(full_path);
            } else {
                abyssfs_unlink(full_path);
            }
        }
    }
    
    
    return abyssfs_unlink(path);
}