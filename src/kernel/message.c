#include "message.h"
#include "process.h"
#include "uart.h"
#include "vfs.h"
#include "string.h"


int queue_message(struct process *proc, struct Message *msg) {
    struct message_queue *queue = &proc->msg_queue;
    
    if (queue->count >= MAX_MESSAGES) {
        return -1;  
    }

    memcpy(&queue->messages[queue->tail], msg, sizeof(struct Message));
    queue->tail = (queue->tail + 1) % MAX_MESSAGES;
    queue->count++;
    
    
    if (proc->msg_blocked) {
        proc->msg_blocked = 0;
        
    }
    
    return 0;
}

int send_message(struct Message *msg) {
    switch(msg->type) {
        case MSG_OPEN: {
            int fd = vfs_open(msg->path);
            msg->fd = fd;  
            return fd;
        }
        case MSG_READ: {
            uart_puts("Reading from fd: ");
            uart_hex(msg->fd);
            uart_puts("\n");
            
            char buf[256];
            ssize_t bytes = vfs_read(msg->fd, buf, sizeof(buf));
            if (bytes > 0) {
                memcpy(msg->data, buf, bytes);
                msg->size = bytes;
            }
            return bytes;
        }
        case MSG_FORK: {
            struct process *new = create_process();
            if (!new) return -1;
            
            
            msg->pid = new->pid;  
            
            
            if (new == get_current_process()) {
                return 0;
            }
            
            
            return 0;
        }
        case MSG_EXEC: {
            return handle_exec_message(msg);
        }
        case MSG_WAIT: {
            struct process *current = get_current_process();
            uart_puts("Parent (PID ");
            uart_hex(current->pid);
            uart_puts(") waiting for children\n");
            
            
            struct process *p = process_list;
            while (p) {
                if (p->parent_pid == current->pid && p->state == PROC_ZOMBIE) {
                    msg->status = p->exit_status;
                    uart_puts("Found zombie child, status: ");
                    uart_hex(msg->status);
                    uart_puts("\n");
                    return 0;
                }
                p = p->next;
            }
            
            
            current->state = PROC_BLOCKED;
            schedule();
            
            
            p = process_list;
            while (p) {
                if (p->parent_pid == current->pid && p->state == PROC_ZOMBIE) {
                    msg->status = p->exit_status;
                    uart_puts("Child exited with status: ");
                    uart_hex(msg->status);
                    uart_puts("\n");
                    return 0;
                }
                p = p->next;
            }
            
            return -1;
        }
        case MSG_PIPE: {
            
            
            return 0;
        }
        case MSG_READ_DIR:
            uart_puts("DEBUG: MSG_READ_DIR received\n");
            return handle_read_dir_message(msg);
        case MSG_CREATE: {
            uart_puts("DEBUG: MSG_CREATE received for path: ");
            uart_puts(msg->path);
            uart_puts("\n");
            int fd = vfs_create(msg->path);
            if (fd >= 0) {
                vfs_close(fd);  // close immediately like touch does
                msg->fd = fd;   // Store fd in message
                return 0;       // success
            }
            return -1;          // fail
        }
        case MSG_MKDIR: {
            uart_puts("DEBUG: MSG_MKDIR received for path: ");
            uart_puts(msg->path);
            uart_puts("\n");
            int result = vfs_mkdir(msg->path);
            return result;      // return mkdir result directly
        }
        case MSG_GETCWD: {
            struct process *current = get_current_process();
            if (!current) {
                return -1;
            }
            // copy current working directory to the message data
            if (msg->data && msg->size > 0) {
                size_t cwd_len = strlen(current->cwd);
                if (cwd_len < msg->size) {
                    strcpy((char*)msg->data, current->cwd);
                    msg->size = cwd_len;
                    return 0;
                } else {
                    return -1;  // buffer too small
                }
            }
            return -1;
        }
        case MSG_CHDIR: {
            process_t *current = get_current_process();
            if (!current) {
                msg->status = -1;
                return -1;
            }

            // full path
            char full_path[256];
            if (msg->path[0] == '/') {
                strncpy(full_path, msg->path, sizeof(full_path) - 1);
            } else {
                strcpy(full_path, current->cwd);
                if (full_path[strlen(full_path) - 1] != '/') {
                    strcat(full_path, "/");
                }
                strcat(full_path, msg->path);
            }
            full_path[sizeof(full_path) - 1] = '\0';

            // handle ..
            uart_puts("DEBUG: Before normalization: "); uart_puts(full_path); uart_puts("\n");
            vfs_normalize_path(full_path);
            uart_puts("DEBUG: After normalization: "); uart_puts(full_path); uart_puts("\n");

            // if directory exists and is accessible
            struct dirent dirents[1];
            int count = vfs_read_dir(full_path, dirents, 1);
            if (count >= 0) {
                strncpy(current->cwd, full_path, sizeof(current->cwd) - 1);
                current->cwd[sizeof(current->cwd) - 1] = '\0';
                uart_puts("DEBUG: Updated cwd to: "); uart_puts(current->cwd); uart_puts("\n");
                msg->status = 0;
            } else {
                uart_puts("DEBUG: Directory not accessible: "); uart_puts(full_path); uart_puts("\n");
                msg->status = -1;
            }
            return msg->status;
        }
        case MSG_COPY: {
            uart_puts("DEBUG: MSG_COPY received\n");
            
            if (!msg->path || !msg->data) {
                uart_puts("DEBUG: Missing source or destination path\n");
                return -1;
            }
            
            char *src_path = msg->path;
            char *dst_path = (char *)msg->data;
            
            uart_puts("DEBUG: Copying from: ");
            uart_puts(src_path);
            uart_puts(" to: ");
            uart_puts(dst_path);
            uart_puts("\n");
            
            // read source file
            int src_fd = vfs_open(src_path);
            if (src_fd < 0) {
                uart_puts("DEBUG: Cannot open source file\n");
                return -1;
            }
            
            // create destination file
            int dst_fd = vfs_create(dst_path);
            if (dst_fd < 0) {
                uart_puts("DEBUG: Cannot create destination file\n");
                vfs_close(src_fd);
                return -1;
            }
            
            // copy data in chunks
            char buffer[1024];
            int bytes_read;
            int total_copied = 0;
            
            while ((bytes_read = vfs_read(src_fd, buffer, sizeof(buffer))) > 0) {
                int bytes_written = vfs_write(dst_fd, buffer, bytes_read);
                if (bytes_written != bytes_read) {
                    uart_puts("DEBUG: Write error during copy\n");
                    vfs_close(src_fd);
                    vfs_close(dst_fd);
                    return -1;
                }
                total_copied += bytes_written;
            }
            
            vfs_close(src_fd);
            vfs_close(dst_fd);
            
            uart_puts("DEBUG: Copy completed, bytes copied: ");
            uart_hex(total_copied);
            uart_puts("\n");
            
            return 0;
        }
        case MSG_REMOVE: {
            uart_puts("DEBUG: MSG_REMOVE received for path: ");
            uart_puts(msg->path);
            uart_puts("\n");
            
            if (!msg->path) {
                uart_puts("DEBUG: No path provided for removal\n");
                return -1;
            }
            
            // use VFS unlink function to remove the file
            int result = vfs_unlink(msg->path);
            if (result < 0) {
                uart_puts("DEBUG: Failed to remove file\n");
                return -1;
            }
            
            uart_puts("DEBUG: File removed successfully\n");
            return 0;
        }
        case MSG_MOVE: {
            uart_puts("DEBUG: MSG_MOVE received\n");
            
            if (!msg->path || !msg->data) {
                uart_puts("DEBUG: Missing source or destination path\n");
                return -1;
            }
            
            char *src_path = msg->path;
            char *dst_path = (char *)msg->data;
            
            uart_puts("DEBUG: Moving from: ");
            uart_puts(src_path);
            uart_puts(" to: ");
            uart_puts(dst_path);
            uart_puts("\n");
            
            // no vfs_rename, so implementation is copy + remove
            
            // try to copy the file
            int src_fd = vfs_open(src_path);
            if (src_fd < 0) {
                uart_puts("DEBUG: Cannot open source file for move\n");
                return -1;
            }
            
            int dst_fd = vfs_create(dst_path);
            if (dst_fd < 0) {
                uart_puts("DEBUG: Cannot create destination file for move\n");
                vfs_close(src_fd);
                return -1;
            }
            
            // copy data
            char buffer[1024];
            int bytes_read;
            int total_copied = 0;
            
            while ((bytes_read = vfs_read(src_fd, buffer, sizeof(buffer))) > 0) {
                int bytes_written = vfs_write(dst_fd, buffer, bytes_read);
                if (bytes_written != bytes_read) {
                    uart_puts("DEBUG: Write error during move\n");
                    vfs_close(src_fd);
                    vfs_close(dst_fd);
                    vfs_unlink(dst_path);
                    return -1;
                }
                total_copied += bytes_written;
            }
            
            vfs_close(src_fd);
            vfs_close(dst_fd);
            
            int unlink_result = vfs_unlink(src_path);
            if (unlink_result < 0) {
                uart_puts("DEBUG: Failed to remove source file after copy\n");
                // clean up destination file since move failed
                vfs_unlink(dst_path);
                return -1;
            }
            
            uart_puts("DEBUG: Move completed, bytes moved: ");
            uart_hex(total_copied);
            uart_puts("\n");
            
            return 0;
        }
        case MSG_BIND: {
            uart_puts("DEBUG: MSG_BIND received\n");
            
            if (!msg->path || !msg->data) {
                uart_puts("DEBUG: Missing old or new path for bind\n");
                return -1;
            }
            
            char *source_path = msg->path;       
            char *target_path = (char *)msg->data;  
            
            uart_puts("DEBUG: Binding ");
            uart_puts(target_path);
            uart_puts(" -> ");
            uart_puts(source_path);
            uart_puts("\n");
            
            // bind function: bind(source, target) makes target show source contents
            int result = bind(target_path, source_path, 16); // MREPL = 16
            if (result < 0) {
                uart_puts("DEBUG: bind() failed\n");
                return -1;
            }
            
            uart_puts("DEBUG: bind() succeeded\n");
            return 0;
        }
        case MSG_UNBIND:
            return unbind(msg->path);
        case MSG_PUTC: {
            uart_putc(msg->character);
            return 0;
        }
        case MSG_GETC: {
            char c = uart_getc();
            msg->character = c;
            return (int)c;
        }
        case MSG_PUTS: {
            uart_puts(msg->string);
            return 0;
        }
        default:
            return -1;
    }
}

int receive_message(struct Message *msg) {
    struct process *current = get_current_process();
    struct message_queue *queue = &current->msg_queue;
    
    if (queue->count == 0) {
        
        if (!(msg->flags & MSG_NONBLOCK)) {
            current->msg_blocked = 1;
            
            schedule();  
        }
        return -1;
    }
    
    
    memcpy(msg, &queue->messages[queue->head], sizeof(struct Message));
    queue->head = (queue->head + 1) % MAX_MESSAGES;
    queue->count--;
    
    return 0;
}

int handle_message(struct Message *msg) {
    switch (msg->type) {
        case MSG_OPEN:
            return send_message(msg);
        case MSG_READ:
            return send_message(msg);
        case MSG_FORK:
            return send_message(msg);
        case MSG_EXEC:
            return send_message(msg);
        case MSG_WAIT:
            return send_message(msg);
        case MSG_PIPE:
            return send_message(msg);
        case MSG_PUTC:
            return send_message(msg);
        case MSG_GETC:
            return send_message(msg);
        case MSG_PUTS:
            return send_message(msg);
        default:
            uart_puts("Unknown message type: ");
            uart_hex(msg->type);
            uart_puts("\n");
            return -1;
    }
}