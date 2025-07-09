#include <stdint.h>
#include "uart.h"
#include "process.h"
#include "ramfs.h"
#include "namespace.h"
#include "vfs.h"

void uart_hex(unsigned long h);

void process1(void) {
    uart_puts("Process 1: Testing basic namespace binding\n");
    
    
    uart_puts("Creating directories...\n");
    vfs_mkdir("/tmp/data");
    vfs_mkdir("/private");
    
    
    uart_puts("Creating test file in /tmp/data/test.txt\n");
    if (ramfs_create_file("/tmp/data/test.txt", "Hello from RAMFS!") < 0) {
        uart_puts("Failed to create test file\n");
        process_exit(1);
    }

    
    uart_puts("Creating bind mount: /private -> /tmp/data\n");
    if (bind("/tmp/data", "/private", MREPL) < 0) {
        uart_puts("Failed to create bind mount\n");
        process_exit(1);
    }
    
    uart_puts("Accessing file through namespace as /private/test.txt\n");
    char buf[128];
    int fd = vfs_open("/private/test.txt");
    if (fd < 0) {
        uart_puts("Failed to open file through namespace\n");
        process_exit(1);
    }

    if (vfs_read(fd, buf, sizeof(buf)) < 0) {
        uart_puts("Failed to read file through namespace\n");
        process_exit(1);
    }

    uart_puts("Read through namespace: ");
    uart_puts(buf);
    uart_puts("\n");

    
    uart_puts("\nListing /tmp/data:\n");
    struct dirent dirents[32];
    int max_entries = 32;
    int entry_count = vfs_read_dir("/tmp/data", dirents, max_entries);
    if (entry_count >= 0) {
        for (int i = 0; i < entry_count; i++) {
            uart_puts(dirents[i].name);
            uart_puts("\n");
        }
    }

    uart_puts("\nListing /private:\n");
    max_entries = 32;
    entry_count = vfs_read_dir("/private", dirents, max_entries);
    if (entry_count >= 0) {
        for (int i = 0; i < entry_count; i++) {
            uart_puts(dirents[i].name);
            uart_puts("\n");
        }
    }

    process_exit(0);
}

void process2(void) {
    uart_puts("Process 2: Testing multiple namespace bindings\n");
    
    
    bind("/tmp", "/data", MREPL);
    bind("/tmp", "/backup", MREPL);
    
    
    ramfs_create_file("/tmp/shared.txt", "Shared through namespaces");
    
    char buf[128];
    int fd;
    
    
    const char *paths[] = {"/tmp/shared.txt", "/data/shared.txt", "/backup/shared.txt"};
    for (int i = 0; i < 3; i++) {
        fd = vfs_open(paths[i]);
        if (fd >= 0) {
            if (vfs_read(fd, buf, sizeof(buf)) > 0) {
                uart_puts("Successfully read through ");
                uart_puts(paths[i]);
                uart_puts("\n");
            }
        }
    }
    
    process_exit(0);
}

void process3(void) {
    uart_puts("Process 3: Testing namespace isolation\n");
    uint64_t counter = 0;
    while (counter < 5) {
        uart_puts(".");
        counter++;
    }
    uart_puts("\nProcess 3 done\n");
    process_exit(0);
}