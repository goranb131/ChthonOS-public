/* process.h */
#ifndef _PROCESS_H
#define _PROCESS_H

#include "namespace.h"
#include "message.h"
#include "vfs.h"


typedef unsigned char uint8_t;


#define MAX_PROCESSES     4
#define PROCESS_STACK_SIZE 4096
#define PROCESS_LOAD_ADDR 0x40100000
#define PROCESS_STACK_TOP 0x40000000  

typedef int pid_t;


enum process_state {
    PROC_READY,
    PROC_RUNNING,
    PROC_BLOCKED,
    PROC_ZOMBIE,
    PROC_DEAD
};


typedef struct process process_t;

typedef struct context {
    unsigned long pc;
    unsigned long sp;
    unsigned long lr;
    unsigned long x[31];  
} context_t;

#define MAX_MOUNTS 16  

struct mount_point {
    char source[256];
    char target[256];
    int flags;
};

struct process {
    pid_t pid;
    pid_t parent_pid;
    enum process_state state;
    struct proc_namespace ns;
    context_t ctx;
    unsigned long sp;  
    struct process *next;
    int exit_status;
    struct message_queue msg_queue;
    int msg_blocked;
    char cwd[VFS_MAX_PATH];  
};

typedef struct process process_t;

/* functions to initialize process management, create processes, and schedule */
void process_init(void);
process_t* process_create(void (*entry)(void));
void schedule(void);
void process_exit(int status);

/* global pointer to the currently running process */
extern process_t *current_process;
extern struct process *process_list;  


struct process* get_current_process(void);


void init_process(void);
struct process* create_process(void);  
pid_t get_pid(struct process *p);
void exit_process(int status);  
struct process* find_process(int pid);  


void switch_to_process(struct process *next);
void context_switch(context_t *old_ctx, context_t *new_ctx);


void timer_clear_interrupt(void);
void enable_timer_interrupt(void);
void timer_init(void);
void gic_init(void);

int handle_exec_message(struct Message *msg);

#endif