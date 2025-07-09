#ifndef _MESSAGE_H
#define _MESSAGE_H


struct process;

#include <stddef.h>
#include <stdint.h>  

typedef int pid_t;  


enum msg_type {
    MSG_NONE = 0,
    MSG_OPEN,    
    MSG_READ,    
    MSG_WRITE,   
    MSG_CLOSE,   
    MSG_STAT,    
    MSG_BIND,    
    MSG_MOUNT,   
    MSG_FORK,    
    MSG_EXEC,    
    MSG_WAIT,    
    MSG_PIPE,    
    MSG_READ_DIR,
    MSG_CREATE,  // create new file
    MSG_MKDIR,   // create new directory
    MSG_GETCWD,  // get current working directory
    MSG_CHDIR,   // change current working directory
    MSG_COPY,    // copy file
    MSG_REMOVE,  // remove file
    MSG_MOVE,    // move/rename file
    MSG_UNBIND,
    // console/terminal service messages
    MSG_PUTC,    // put character to console
    MSG_GETC,    // get character from console  
    MSG_PUTS,    // put string to console
};

#define MSG_NONBLOCK 0x01

struct Message {
    uint64_t type;           
    char *path;         
    char **argv;        
    void *data;         
    size_t size;        
    int flags;          
    int fd;             
    pid_t pid;          
    int status;         
    unsigned long entry;  
    struct dirent *dirents;  
    size_t dirent_count;
    // console service fields
    char character;      // for MSG_PUTC/MSG_GETC
    char *string;        // for MSG_PUTS
};


int send_message(struct Message *msg);
int receive_message(struct Message *msg);

#define MAX_MESSAGES 32

struct message_queue {
    struct Message messages[MAX_MESSAGES];
    int head;
    int tail;
    int count;
};


int queue_message(struct process *proc, struct Message *msg);

#endif