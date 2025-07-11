#include "process.h"
#include "uart.h"   
#include "message.h"
#include "namespace.h"
#include "string.h"    
#include "kmalloc.h"   
#include "timer.h"
#include "gic.h"

#define MAX_PROCESSES    4
#define PROCESS_STACK_SIZE 4096

static process_t processes[MAX_PROCESSES];
static int process_count = 0;
static uint8_t process_stacks[MAX_PROCESSES][PROCESS_STACK_SIZE];

static process_t *ready_queue = NULL;

process_t *current_process = NULL;
struct process *process_list = NULL;  

static int next_pid = 2;  

typedef unsigned long uint64_t;

void uart_hex(unsigned long h);

void process_init(void) {
    uart_puts("Initializing process management...\n");
    process_count = 0;
    current_process = NULL;
    ready_queue = NULL;
}


process_t* process_create(void (*entry)(void)) {
    process_t *proc = kalloc(sizeof(process_t));
    if (!proc) {
        uart_puts("Failed to allocate process\n");
        return NULL;
    }
    
    init_process_namespace(&proc->ns);
    
    if (!current_process) {
        current_process = proc;
    }

    if (!process_list) {
        process_list = proc;
    } else {
        proc->next = process_list;
        process_list = proc;
    }

    proc->ctx.pc = (unsigned long)entry;
    proc->pid = next_pid++;

    // init current working directory to root
    proc->cwd[0] = '/';
    proc->cwd[1] = '\0';

    return proc;
}

extern void switch_context(context_t *old_ctx, context_t *new_ctx);

void schedule(void) {
    struct process *current = get_current_process();
    struct process *next = NULL;
    
    for (next = process_list; next; next = next->next) {
        
        if (next->state == PROC_ZOMBIE) {
            continue;
        }
        
        if (next->state == PROC_READY) {
            // uart_puts("Schedule: switching from PID ");
            // uart_hex(current->pid);
            // uart_puts(" to PID ");
            // uart_hex(next->pid);
            // uart_puts("\n");
            
            next->state = PROC_RUNNING;
            if (current && current->state == PROC_RUNNING) {
                current->state = PROC_READY;
            }

            struct process *old = current_process;
            current_process = next;
            
            if (old) {
                context_switch(&old->ctx, &next->ctx);
            } else {
                context_switch(NULL, &next->ctx);
            }
            return;
        }
    }
    
    uart_puts("No ready processes to run\n");
    while(1); 
}

void process_exit(int status) {
    if (!current_process) return;
    
    uart_puts("Process ");
    uart_hex(current_process->pid);
    uart_puts(" exiting with status: ");
    uart_hex(status);
    uart_puts("\n");
    
    current_process->state = PROC_ZOMBIE;
    current_process->exit_status = status;
    
    process_t *prev = NULL;
    process_t *temp = ready_queue;
    while (temp) {
        if (temp == current_process) {
            if (prev) {
                prev->next = temp->next;
            } else {
                ready_queue = temp->next;
            }
            break;
        }
        prev = temp;
        temp = temp->next;
    }
    
    current_process = NULL;
    
    schedule();  
}

pid_t fork(void);
pid_t wait(int *status);
int execve(const char *path, char *const argv[], char *const envp[]);

void init_process(void) {
    
    struct process *init = &processes[0];
    init->pid = 1;  
    init->parent_pid = 0;  
    init->state = PROC_RUNNING;
    init->next = NULL;
    
    
    init->sp = (unsigned long)&process_stacks[0][PROCESS_STACK_SIZE];
    init->ctx.sp = init->sp;
    init->ctx.lr = 0;  
    
    
    process_list = init;
    current_process = init;
    process_count = 1;
    next_pid = 2;  
    
    uart_puts("Process management initialized\n");
}

struct process* get_current_process(void) {
    return current_process;
}

pid_t get_pid(struct process *p) {
    return p->pid;
}


struct process* create_process(void) {
    struct process *current = get_current_process();
    struct process *new = &processes[process_count++];
    
    uart_puts("Creating process with PID: ");
    uart_hex(next_pid);
    uart_puts("\n");
    
    new->pid = next_pid++;
    new->parent_pid = current->pid;
    new->state = PROC_READY;
    new->next = process_list;
    process_list = new;
    
    new->sp = (unsigned long)&process_stacks[process_count-1][PROCESS_STACK_SIZE];
    memcpy(&new->ctx, &current->ctx, sizeof(context_t));
    new->ctx.sp = new->sp;
    
    extern void process3(void);  
    new->ctx.lr = (unsigned long)process3;
    
    uart_puts("New context: LR=");
    uart_hex(new->ctx.lr);
    uart_puts("\n");
    
    return new;
}

int load_program(struct process *proc, const char *path) {
    struct Message msg = {
        .type = MSG_OPEN,
        .path = (char*)path,
        .flags = 0
    };
    
    if (send_message(&msg) < 0) {
        return -1;
    }
    
    msg.type = MSG_READ;
    msg.fd = msg.status;  
    msg.data = (void*)proc->ctx.pc;  
    msg.size = 4096;  
    
    if (send_message(&msg) < 0) {
        return -1;
    }
    
    msg.type = MSG_CLOSE;
    send_message(&msg);
    
    return 0;
}

int exec_process(char *path) {
    struct process *current = get_current_process();
    
    
    if (load_program(current, path) < 0) {
        return -1;
    }
    
    current->sp = (unsigned long)&process_stacks[process_count-1][PROCESS_STACK_SIZE];

    current->ctx.pc = PROCESS_LOAD_ADDR;  
    
    return 0;
}

int wait_for_child(int *status) {
    struct process *current = get_current_process();
    current->state = PROC_BLOCKED;
    schedule();  
    return 0;
}

struct process* find_process(int pid) {
    struct process *p = process_list;
    while (p) {
        if (p->pid == pid) {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

void exit_process(int status) {
    struct process *current = get_current_process();
    
    uart_puts("Process ");
    uart_hex(current->pid);
    uart_puts(" exiting with status: ");
    uart_hex(status);
    uart_puts("\n");
    
    current->state = PROC_ZOMBIE;
    current->exit_status = status;
    
    struct process *parent = find_process(current->parent_pid);
    if (parent && parent->state == PROC_BLOCKED) {
        parent->state = PROC_READY;
    }
    
    schedule();
}


void switch_to_process(struct process *next) {
    struct process *prev = current_process;
    current_process = next;
    context_switch(&prev->ctx, &next->ctx);
}


void init_processes(void) {
    
    processes[0].pid = 1;
    processes[0].state = PROC_RUNNING;
    current_process = &processes[0];
    process_list = current_process;
    
    timer_init();
    gic_init();
    enable_timer_interrupt();
    
    uart_puts("Process management initialized\n");
}


int handle_fork_message(struct Message *msg) {
    struct process *current = get_current_process();
    struct process *new = create_process();
    if (!new) {
        msg->status = -1;
        return -1;
    }
    
    new->parent_pid = current->pid;
    new->state = PROC_READY;
    
    memcpy(&new->ctx, &current->ctx, sizeof(context_t));
    new->ctx.x[0] = 0;  
    new->ctx.pc = msg->entry;  
    
    uart_puts("Created process PID: ");
    uart_hex(new->pid);
    uart_puts("\n");
    
    msg->pid = new->pid;
    msg->status = 0;
    return 0;
}


int handle_wait_message(struct Message *msg) {
    struct process *current = get_current_process();
    
    struct process *p;
    for (p = process_list; p; p = p->next) {
        if (p->parent_pid == current->pid && p->state == PROC_ZOMBIE) {
            msg->status = p->exit_status;
            msg->pid = p->pid;
            p->state = PROC_DEAD;  
            return 0;
        }
    }
    
    current->state = PROC_BLOCKED;
    msg->status = 0;
    schedule();
    return 0;
}

int handle_exec_message(struct Message *msg) {
    if (!msg->path) {
        return -1;
    }
    
    uart_puts("Executing program: ");
    uart_puts(msg->path);
    uart_puts("\n");
    
    struct process *current = get_current_process();
    current->ctx.pc = msg->entry;
    current->ctx.sp = PROCESS_STACK_TOP;
    
    
    for (int i = 0; i < 31; i++) {
        current->ctx.x[i] = 0;
    }
    
    
    if (msg->argv) {
        int argc = 0;
        while (msg->argv[argc]) argc++;
        current->ctx.x[0] = argc;
        current->ctx.x[1] = (uint64_t)msg->argv;
    }
    
    return 0;
}