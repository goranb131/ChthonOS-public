#include "namespace.h"
#include "uart.h"
#include "kmalloc.h"
#include "string.h"
#include "process.h"
#include "vfs.h"

static struct namespace *mounts = NULL;  

struct namespace* create_namespace(const char *old, const char *new, int flags) {
    struct namespace *ns = kalloc(sizeof(struct namespace));
    if (!ns) {
        uart_puts("Failed to allocate namespace\n");
        return NULL;
    }
    
    size_t old_len = strlen(old) + 1;
    size_t new_len = strlen(new) + 1;
    
    ns->old_path = kalloc(old_len);
    ns->new_path = kalloc(new_len);
    
    if (!ns->old_path || !ns->new_path) {
        uart_puts("Failed to allocate namespace paths\n");
        kfree(ns);
        return NULL;
    }
    
    memcpy(ns->old_path, old, old_len);
    memcpy(ns->new_path, new, new_len);
    ns->flags = flags;
    ns->next = NULL;
    
    return ns;
}


void init_process_namespace(struct proc_namespace *ns) {
    ns->mounts = NULL;
}

int bind(const char *old, const char *new, int flags) {
    uart_puts("Binding ");
    uart_puts(new);
    uart_puts(" to ");
    uart_puts(old);
    uart_puts("\n");
    
    
    struct dirent dirents[1];
    int max_entries = 1;
    if (vfs_read_dir(old, dirents, max_entries) < 0) {
        uart_puts("Source directory does not exist\n");
        return -1;
    }
    
    if (vfs_read_dir(new, dirents, max_entries) < 0) {
        uart_puts("Target directory does not exist\n");
        return -1;
    }
    
    
    struct namespace *ns = create_namespace(old, new, flags);
    if (!ns) {
        return -1;
    }
    
    if (current_process) {
        ns->next = current_process->ns.mounts;
        current_process->ns.mounts = ns;
        uart_puts("Added namespace binding to process\n");
    }
    
    return 0;
}

int mount(const char *srv, const char *old, int flags, const char *spec) {
    
    return 0;
}

int unbind(const char *path) {
    if (!current_process) {
        uart_puts("No current process\n");
        return -1;
    }

    struct namespace *entry = current_process->ns.mounts;
    struct namespace *prev = NULL;

    uart_puts("Unbinding namespace at: ");
    uart_puts(path);
    uart_puts("\n");

    while (entry) {
        uart_puts("Checking mount: ");
        uart_puts(entry->new_path);
        uart_puts(" -> ");
        uart_puts(entry->old_path);
        uart_puts("\n");

        if (strcmp(entry->new_path, path) == 0) {
            if (prev) {
                prev->next = entry->next;
            } else {
                current_process->ns.mounts = entry->next;
            }
            kfree(entry->old_path);
            kfree(entry->new_path);
            kfree(entry);
            uart_puts("Successfully unbound namespace\n");
            return 0;
        }
        prev = entry;
        entry = entry->next;
    }

    uart_puts("Mount point not found\n");
    return -1;
} 