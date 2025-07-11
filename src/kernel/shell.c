#include "message.h"
#include "uart.h"
#include "process.h"  
#include "string.h"   
#include "vfs.h"     
#include <stddef.h>

#define MAX_INPUT 256
#define PROMPT "$ "


static void cmd_exit(void) {
    exit_process(0);
}

static void cmd_echo(char *args) {
    if (args) {
        uart_puts(args);
    }
    uart_puts("\n");
}


typedef void (*cmd_func)(char *args);


void cmd_ls(char *args);
void cmd_pwd(char *args);
void cmd_cd(char *path);
void cmd_cp(char *args);
void cmd_touch(char *args);
void cmd_mv(char *args);
void cmd_rm(char *args);
void cmd_mkdir(char *args);
void cmd_bind(char *args);


extern struct process* current_process;  

static struct {
    const char *name;
    cmd_func func;
} commands[] = {
    {"ls", cmd_ls},
    {"pwd", cmd_pwd},
    {"cd", cmd_cd},
    {"cp", cmd_cp},
    {"mv", cmd_mv},
    {"touch", cmd_touch},
    {"rm", cmd_rm},
    {"mkdir", cmd_mkdir},
    {"bind", cmd_bind},
    {NULL, NULL}
};


void cmd_pwd(char *args) {
    if (current_process->cwd[0] == '\0') {
        uart_puts("/\n");
    } else {
        uart_puts(current_process->cwd);
        uart_puts("\n");
    }
}


static void get_parent_dir(char *path) {
    
    if (strcmp(path, "/") == 0) {
        return;
    }
    
    int len = strlen(path);
    
    
    if (len > 1 && path[len-1] == '/') {
        len--;
        path[len] = '\0';
    }
    
    
    while (len > 0 && path[len-1] != '/') {
        len--;
    }
    
    
    if (len > 0) {
        
        if (len == 1) {
            path[1] = '\0';
        } else {
            path[len-1] = '\0';  
        }
    }
    
    
    if (path[0] == '\0') {
        path[0] = '/';
        path[1] = '\0';
    }
}


static void normalize_path(char *path) {
    char *src = path;
    char *dst = path;
    
    
    if (*src == '/') {
        *dst++ = *src++;
    }
    
    
    while (*src) {
        if (*src == '/' && *(src + 1) == '/') {
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

void cmd_cd(char *path) {
    if (!path || !*path) {
        current_process->cwd[0] = '/';
        current_process->cwd[1] = '\0';
        return;
    }

    
    if (strcmp(path, ".") == 0) {
        return;
    }

    if (strcmp(path, "..") == 0) {
        get_parent_dir(current_process->cwd);
        return;
    }

    
    char temp_path[VFS_MAX_PATH];
    
    
    if (path[0] == '/') {
        temp_path[0] = '/';
        temp_path[1] = '\0';
        path++;  
    } else {
        
        strcpy(temp_path, current_process->cwd);
    }

    
    char *component;
    char *saveptr;
    char path_copy[VFS_MAX_PATH];
    strcpy(path_copy, path);
    
    component = strtok_r(path_copy, "/", &saveptr);
    while (component) {
        if (strcmp(component, ".") == 0) {
            
        } else if (strcmp(component, "..") == 0) {
            get_parent_dir(temp_path);
        } else {
            
            if (strcmp(temp_path, "/") != 0) {
                strcat(temp_path, "/");
            }
            strcat(temp_path, component);
        }
        component = strtok_r(NULL, "/", &saveptr);
    }

    
    int fd = vfs_open(temp_path);
    if (fd < 0) {
        uart_puts("cd: no such directory: ");
        uart_puts(temp_path);
        uart_puts("\n");
        return;
    }
    
    
    strcpy(current_process->cwd, temp_path);
}

void cmd_ls(char *path) {
    
    if (!path || !*path) {
        path = current_process->cwd;
    }
    
    struct Message msg = {
        .type = MSG_READ_DIR,
        .path = path
    };
    
    if (send_message(&msg) >= 0) {
        
        for (size_t i = 0; i < msg.dirent_count; i++) {
            uart_puts(msg.dirents[i].name);
            uart_puts("\n");
        }
    } else {
        uart_puts("ls: cannot access '");
        uart_puts(path);
        uart_puts("'\n");
    }
}

void cmd_cp(char *args) {
    char src[VFS_MAX_PATH];
    char dst[VFS_MAX_PATH];
    
    
    char *saveptr;
    char *src_arg = strtok_r(args, " ", &saveptr);
    char *dst_arg = strtok_r(NULL, " ", &saveptr);
    
    if (!src_arg || !dst_arg) {
        uart_puts("Usage: cp source dest\n");
        return;
    }
    
    strcpy(src, src_arg);
    strcpy(dst, dst_arg);
    
    
    int src_fd = vfs_open(src);
    if (src_fd < 0) {
        uart_puts("cp: cannot open source file: ");
        uart_puts(src);
        uart_puts("\n");
        return;
    }
    
    
    int dst_fd = vfs_create(dst);
    if (dst_fd < 0) {
        uart_puts("cp: cannot create destination file: ");
        uart_puts(dst);
        uart_puts("\n");
        return;
    }
    
    
    char buf[1024];
    ssize_t n;
    while ((n = vfs_read(src_fd, buf, sizeof(buf))) > 0) {
        ssize_t written = vfs_write(dst_fd, buf, n);
        if (written < 0 || written != n) {
            uart_puts("cp: write error\n");
            break;
        }
    }
    
    if (n < 0) {
        uart_puts("cp: read error\n");
    }
}

void cmd_touch(char *args) {
    if (!args || !*args) {
        uart_puts("touch: missing file operand\n");
        return;
    }

    int fd = vfs_create(args);  
    if (fd < 0) {
        uart_puts("touch: cannot create file '");
        uart_puts(args);
        uart_puts("'\n");
        return;
    }
    
    
    vfs_close(fd);
}

void cmd_mv(char *args) {
    char src[VFS_MAX_PATH];
    char dst[VFS_MAX_PATH];
    char cp_args[VFS_MAX_PATH * 2];  
    
    
    char *saveptr;
    char *src_arg = strtok_r(args, " ", &saveptr);
    char *dst_arg = strtok_r(NULL, " ", &saveptr);
    
    if (!src_arg || !dst_arg) {
        uart_puts("Usage: mv source dest\n");
        return;
    }
    
    strcpy(src, src_arg);
    strcpy(dst, dst_arg);
    
    
    strcpy(cp_args, src);
    strcat(cp_args, " ");
    strcat(cp_args, dst);
    
    
    cmd_cp(cp_args);
    
    
    if (vfs_unlink(src) < 0) {
        uart_puts("mv: cannot remove source file\n");
    }
}

void cmd_rm(char *args) {
    if (!args || !*args) {
        uart_puts("Usage: rm [-r] <file|directory>\n");
        return;
    }

    int recursive = 0;
    char *target = NULL;

    
    
    char *saveptr;
    char *token = strtok_r(args, " ", &saveptr);
    if (!token) {
        uart_puts("rm: missing file operand\n");
        return;
    }

    
    if (strcmp(token, "-r") == 0) {
        recursive = 1;
        
        token = strtok_r(NULL, " ", &saveptr);
        if (!token) {
            uart_puts("rm: missing file operand\n");
            return;
        }
    }
    target = token;

    if (recursive) {
        if (vfs_remove_recursive(target) < 0) {
            uart_puts("rm: cannot recursively remove '");
            uart_puts(target);
            uart_puts("'\n");
        }
    } else {
        if (vfs_unlink(target) < 0) {
            uart_puts("rm: cannot remove '");
            uart_puts(target);
            uart_puts("'\n");
        }
    }
}

void cmd_mkdir(char *args) {
    if (!args || !*args) {
        uart_puts("Usage: mkdir <directory>\n");
        return;
    }

    if (vfs_mkdir(args) < 0) {
        uart_puts("mkdir: cannot create directory '");
        uart_puts(args);
        uart_puts("'\n");
    }
}

void cmd_bind(char *args) {
    char *old_path = NULL;
    char *new_path = NULL;
    
    
    char *saveptr;
    old_path = strtok_r(args, " ", &saveptr);
    if (old_path) {
        new_path = strtok_r(NULL, " ", &saveptr);
    }
    
    if (!old_path || !new_path) {
        uart_puts("Usage: bind source target\n");
        return;
    }
    
    
    if (bind(old_path, new_path, MREPL) < 0) {
        uart_puts("bind: failed to create binding\n");
        return;
    }
    
    uart_puts("Created binding: ");
    uart_puts(new_path);
    uart_puts(" -> ");
    uart_puts(old_path);
    uart_puts("\n");
}

void shell(void) {
    char input[MAX_INPUT];
    int pos = 0;
    
    while (1) {
        
        uart_puts(PROMPT);
        
        
        pos = 0;
        while (pos < MAX_INPUT - 1) {
            char c = uart_getc();
            
            if (c == '\r' || c == '\n') {
                input[pos] = '\0';
                uart_puts("\n");
                break;
            } else if ((c == 127 || c == '\b') && pos > 0) {  
                pos--;
                uart_puts("\b \b");  
            } else if (c >= ' ' && c <= '~') {
                input[pos++] = c;
                uart_putc(c);  
            }
        }
        
        
        char *cmd = input;
        char *args = NULL;
        
        
        for (int i = 0; i < pos; i++) {
            if (input[i] == ' ') {
                input[i] = '\0';
                args = &input[i + 1];
                break;
            }
        }
        
        
        if (strcmp(cmd, "exit") == 0) {
            cmd_exit();
        } else if (strcmp(cmd, "echo") == 0) {
            cmd_echo(args);
        } else if (strcmp(cmd, "pwd") == 0) {
            cmd_pwd(args);
        } else if (strcmp(cmd, "cd") == 0) {
            cmd_cd(args);
        } else if (strcmp(cmd, "ls") == 0) {
            cmd_ls(args);
        } else if (strcmp(cmd, "cp") == 0) {
            cmd_cp(args);
        } else if (strcmp(cmd, "touch") == 0) {
            cmd_touch(args);
        } else if (strcmp(cmd, "mv") == 0) {
            cmd_mv(args);
        } else if (strcmp(cmd, "rm") == 0) {
            cmd_rm(args);
        } else if (strcmp(cmd, "mkdir") == 0) {
            cmd_mkdir(args);
        } else if (strcmp(cmd, "bind") == 0) {
            cmd_bind(args);
        } else if (strcmp(cmd, "unbind") == 0) {
            if (!args) {
                uart_puts("Usage: unbind <path>\n");
            } else {
                struct Message msg = {0};
                msg.type = MSG_UNBIND;
                msg.path = args;
                
                if (send_message(&msg) < 0) {
                    uart_puts("unbind: failed\n");
                } else {
                    uart_puts("Namespace unbound: ");
                    uart_puts(args);
                    uart_puts("\n");
                }
            }
        } else {
            uart_puts("Unknown command: ");
            uart_puts(cmd);
            uart_puts("\n");
        }
    }
} 