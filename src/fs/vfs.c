#include "vfs.h"
#include "uart.h"
#include "kmalloc.h"
#include "string.h"
#include "namespace.h"
#include "process.h"  
#include "abyssfs.h"  
#include "ramfs.h"     


extern struct vfs_file_operations ramfs_fops;
extern struct filesystem_type ramfs_fs_type;
extern struct filesystem_type abyssfs_fs_type;


#define MAX_FS 4
static struct filesystem_type *registered_fs[MAX_FS];
static int num_registered_fs = 0;


struct mount {
    const char *path;
    struct filesystem_type *fs;
    struct vfs_super_block *sb;
};
static struct mount mount_points[MAX_MOUNTS];
static int num_mounts = 0;


#define MAX_FD 32
static struct vfs_file *fd_table[MAX_FD];
static int next_fd = 0;


static char* resolve_namespace_path(const char *path);

void vfs_normalize_path(char *path) {
    char temp[256];
    char *src = path;
    char *dst = temp;
    char *segment_start = NULL;
    
    temp[0] = '\0';
    
    if (*src == '/') {
        *dst++ = *src++;
    }
    
    while (*src) {
        if (*src == '/' && (dst == temp || *(dst-1) == '/')) {
            src++;
            continue;
        }
        
        if (*src == '/') {
            *dst++ = *src++;
            segment_start = dst;
            continue;
        }
        
        if (*src == '.' && (*(src+1) == '/' || *(src+1) == '\0')) {
            src++;
            continue;
        }
        
        if (*src == '.' && *(src+1) == '.' && (*(src+2) == '/' || *(src+2) == '\0')) {
            src += 2;
            
            if (dst > temp + 1) {  // not at root
                dst--; 
                while (dst > temp && *(dst-1) != '/') {
                    dst--;
                }
            }
            continue;
        }
        
        *dst++ = *src++;
    }
    
    if (dst == temp) {
        *dst++ = '/';
    } else if (dst > temp + 1 && *(dst-1) == '/') {
        dst--; 
    }
    *dst = '\0';
    
    strcpy(path, temp);
    
    /*uart_puts("DEBUG: After normalization: "); 
    uart_puts(path); 
    uart_puts("\n");*/
}

void vfs_init(void) {
    uart_puts("Initializing VFS...\n");
    
    abyssfs_init();
    ramfs_init();
    
    vfs_mount("/", &abyssfs_fs_type);      
    vfs_mount("/tmp", &ramfs_fs_type);     
    
    test_abyssfs();
}


int register_filesystem(struct filesystem_type *fs) {
    if (num_registered_fs >= MAX_FS) {
        return -1;
    }
    registered_fs[num_registered_fs++] = fs;
    return 0;
}


int vfs_mount(const char *path, struct filesystem_type *fs) {
    if (num_mounts >= MAX_MOUNTS) {
        return -1;
    }

    struct vfs_super_block *sb = fs->mount();
    if (!sb) {
        return -1;
    }

    mount_points[num_mounts].path = path;
    mount_points[num_mounts].fs = fs;
    mount_points[num_mounts].sb = sb;
    num_mounts++;

    return 0;
}


static struct mount *find_mount(const char *path) {
    if (!path) return NULL;
    
    
    if (path[0] == '\0' || strcmp(path, "/") == 0) {
        
        if (num_mounts > 0) {
            return &mount_points[0];
        }
        return NULL;
    }
    
    
    struct mount *best_mount = NULL;
    size_t best_len = 0;
    
    for (int i = 0; i < num_mounts; i++) {
        size_t mount_len = strlen(mount_points[i].path);
        if (strncmp(path, mount_points[i].path, mount_len) == 0) {
            if (mount_len > best_len) {
                best_len = mount_len;
                best_mount = &mount_points[i];
            }
        }
    }
    
    return best_mount;
}


static int alloc_fd(struct vfs_file *file) {
    
    if (next_fd >= MAX_FD) {
        uart_puts("VFS: No free file descriptors\n");
        return -1;
    }
    fd_table[next_fd] = file;
    
    return next_fd++;
}


static struct vfs_file *get_file(int fd) {
    if (fd < 0 || fd >= MAX_FD) {
        return NULL;
    }
    return fd_table[fd];
}


int vfs_read_dir(const char *path, struct dirent *dirents, int max_entries) {
    //uart_puts("VFS: vfs_read_dir called with path: ");
    //uart_puts(path);
    //uart_puts("\n");

    char *resolved_path = resolve_namespace_path(path);
    //uart_puts("VFS: Reading directory after namespace resolution: ");
    //uart_puts(resolved_path);
    //uart_puts("\n");

    struct mount *mp = find_mount(resolved_path);
    if (!mp) {
        uart_puts("VFS: No mount point found for path\n");
        return -1;
    }

    //uart_puts("VFS: Found mount point, calling filesystem read_dir\n");
    
    return mp->fs->read_dir(resolved_path, dirents, max_entries);
}


static char* resolve_namespace_path(const char *path) {
    if (!current_process) {
        return (char*)path;
    }

    //uart_puts("VFS: Resolving path: ");
    //uart_puts(path);
    //uart_puts("\n");

    struct namespace *ns = current_process->ns.mounts;
    while (ns) {
        //uart_puts("VFS: Checking binding: ");
        //uart_puts(ns->new_path);
        //uart_puts(" -> ");
        //uart_puts(ns->old_path);
        //uart_puts("\n");

        size_t new_len = strlen(ns->new_path);
        if (strncmp(path, ns->new_path, new_len) == 0) {
            char *resolved = kalloc(VFS_MAX_PATH);
            if (!resolved) return (char*)path;
            
            strcpy(resolved, ns->old_path);
            
            if (path[new_len] != '\0') {
                if (path[new_len] != '/' && resolved[strlen(resolved)-1] != '/') {
                    strcat(resolved, "/");
                }
                strcat(resolved, path + new_len);
            }
            
            //uart_puts("VFS: Resolved path: ");
            //uart_puts(resolved);
            //uart_puts("\n");
            
            return resolved;
        }
        ns = ns->next;
    }

    return (char*)path;
}


int vfs_open(const char* path) {
    
    char resolved_path[VFS_MAX_PATH];
    if (resolve_path(path, resolved_path) < 0) {
        return -1;
    }
    
    struct filesystem_type* fs = NULL;
    if (strncmp(resolved_path, "/tmp/", 5) == 0) {
        fs = &ramfs_fs_type;
    } else {
        fs = &abyssfs_fs_type;
    }

    struct vfs_file* file = fs->open(resolved_path);
    if (!file) {
        uart_puts("VFS: Failed to open file\n");
        return -1;
    }

    int fd = alloc_fd(file);
    if (fd < 0) {
        uart_puts("VFS: Failed to allocate fd\n");
        
        return -1;
    }

    return fd;
}


ssize_t vfs_read(int fd, void *buf, size_t count) {
    //uart_puts("VFS: Reading from fd: ");
    //uart_hex(fd);
    //uart_puts("\n");

    if (fd < 0 || fd >= MAX_FD || !fd_table[fd]) {
        uart_puts("VFS: Invalid file descriptor\n");
        return -1;
    }

    struct vfs_file *file = fd_table[fd];
    if (!file->f_ops || !file->f_ops->read) {
        uart_puts("VFS: No read operation\n");
        return -1;
    }

    //uart_puts("VFS: Reading using ");
    //uart_puts(file->f_ops == &ramfs_fops ? "RAMFS" : "AbyssFS");
    //uart_puts("\n");
    
    return file->f_ops->read(file, buf, count);
}


int vfs_write(int fd, const void *buf, size_t count) {
    //uart_puts("VFS: Writing to fd ");
    //uart_hex(fd);
    //uart_puts("\n");
    
    struct vfs_file *file = get_file(fd);
    if (!file) {
        uart_puts("VFS: Invalid file descriptor\n");
        return -1;
    }
    
    if (!file->f_ops || !file->f_ops->write) {
        uart_puts("VFS: No write operation available\n");
        return -1;
    }
    
    return file->f_ops->write(file, buf, count);
}


struct vfs_file* get_fs_file(const char *path) {
    
    struct filesystem_type *fs = NULL;
    if (strncmp(path, "/tmp/", 5) == 0) {
        fs = &ramfs_fs_type;
    } else {
        fs = abyssfs_get_fs_type();  
    }
    
    if (!fs || !fs->open) {
        return NULL;
    }
    
    return fs->open(path);
}


int handle_read_dir_message(struct Message *msg) {
    //uart_puts("VFS: handle_read_dir_message called with path: ");
    //uart_puts(msg->path ? msg->path : "NULL");
    //uart_puts("\n");

    if (!msg->path) {
        uart_puts("VFS: ERROR - msg->path is NULL\n");
        return -1;
    }

    static struct dirent dirents[32];  // static array for kernel use
    int max_entries = 32;

    //uart_puts("VFS: About to call vfs_read_dir\n");
    int entry_count = vfs_read_dir(msg->path, dirents, max_entries);
    //uart_puts("VFS: vfs_read_dir returned ");
    //uart_hex(entry_count);
    //uart_puts(" entries\n");
    
    if (entry_count >= 0) {
        // copy directory entries to user messages data field
        // use the msg->data field to store the directory entries
        // and msg->size to indicate how much space is available
        if (msg->data && msg->size >= entry_count * sizeof(struct dirent)) {
            // copy directory entries to user space
            for (int i = 0; i < entry_count; i++) {
                struct dirent *user_dirents = (struct dirent*)msg->data;
                user_dirents[i] = dirents[i];
            }
            msg->dirent_count = entry_count;
            //uart_puts("VFS: Setting dirent_count to ");
            //uart_hex(entry_count);
            //uart_puts(" at address ");
            //uart_hex((uint64_t)&msg->dirent_count);
            //uart_puts(" (offset from msg = ");
            //uart_hex((uint64_t)&msg->dirent_count - (uint64_t)msg);
            //uart_puts(")\n");
            //uart_puts("VFS: Copied ");
            //uart_hex(entry_count);
            //uart_puts(" entries to user space\n");
        } else {
            uart_puts("VFS: No space provided for directory entries\n");
            msg->dirent_count = 0;
        }
    }

    return entry_count;
}

int resolve_path(const char *path, char *resolved) {
    
    struct namespace *ns = current_process->ns.mounts;
    while (ns) {
        if (strncmp(path, ns->old_path, strlen(ns->old_path)) == 0) {
            strcpy(resolved, ns->new_path);
            strcat(resolved, path + strlen(ns->old_path));

            return 0;
        }
        ns = ns->next;
    }
    
    
    strcpy(resolved, path);
    return 0;
}

int vfs_create(const char *path) {
    char full_path[VFS_MAX_PATH];

    
    if (path[0] != '/') {
        strcpy(full_path, current_process->cwd);
        if (full_path[strlen(full_path)-1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, path);
    } else {
        strcpy(full_path, path);
    }

    
    if (strncmp(full_path, "/tmp/", 5) == 0 || strcmp(full_path, "/tmp") == 0) {
        struct vfs_file *file = ramfs_fs_type.create(full_path);
        if (!file) {
            uart_puts("VFS: Failed to create file in RAMFS\n");
            return -1;
        }
        return alloc_fd(file);
    } else {
        struct mount *mp = find_mount(full_path);
        if (!mp || !mp->fs->create) {
            uart_puts("VFS: No mount point or create operation for path\n");
            return -1;
        }
        struct vfs_file *file = mp->fs->create(full_path);
        if (!file) {
            uart_puts("VFS: Failed to create file in filesystem\n");
            return -1;
        }
        return alloc_fd(file);
    }
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_FD || !fd_table[fd]) {
        return -1;
    }

    struct vfs_file *file = fd_table[fd];
    
    if (file->f_ops && file->f_ops->close) {
        file->f_ops->close(file);
    }
    
    kfree(file);
    fd_table[fd] = NULL;
    
    return 0;
}

int vfs_unlink(const char *path) {
    //uart_puts("VFS: Unlinking file: ");
    //uart_puts(path);
    //uart_puts("\n");
    
    char full_path[VFS_MAX_PATH];
    if (path[0] != '/') {
        
        strcpy(full_path, current_process->cwd);
        if (full_path[strlen(full_path)-1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, path);
    } else {
        strcpy(full_path, path);
    }

    
    struct mount *mp = find_mount(full_path);
    if (!mp) {
        uart_puts("VFS: No mount point found\n");
        return -1;
    }

    return mp->fs->unlink(path);
}

int vfs_mkdir(const char *path) {
    //uart_puts("VFS: Creating directory: ");
    //uart_puts(path);
    //uart_puts("\n");

    
    char full_path[VFS_MAX_PATH];
    if (path[0] != '/') {
        strcpy(full_path, current_process->cwd);
        if (full_path[strlen(full_path)-1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, path);
    } else {
        strcpy(full_path, path);
    }

    
    struct mount *mp = find_mount(full_path);
    if (!mp) {
        uart_puts("VFS: No mount point found\n");
        return -1;
    }

    
    return mp->fs->mkdir(full_path);
}

int vfs_remove_recursive(const char *path) {
    if (!path) {
        return -1;
    }
    
    
    char full_path[VFS_MAX_PATH];
    if (path[0] != '/') {
        strcpy(full_path, current_process->cwd);
        if (full_path[strlen(full_path) - 1] != '/') {
            strcat(full_path, "/");
        }
        strcat(full_path, path);
    } else {
        strcpy(full_path, path);
    }
    
    struct mount *mp = find_mount(full_path);
    if (!mp || !mp->fs->remove_recursive) {
        uart_puts("VFS: Filesystem does not support recursive remove\n");
        return -1;
    }
    
    return mp->fs->remove_recursive(full_path);
}