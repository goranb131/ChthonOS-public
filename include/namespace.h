#ifndef NAMESPACE_H
#define NAMESPACE_H

#include <stddef.h>

enum {
    MAFTER      = 1,    
    MBEFORE     = 2,    
    MCREATE     = 4,    
    MCACHE      = 8,    
    MREPL       = 16    
};


struct namespace {
    char *old_path;     
    char *new_path;     
    int flags;          
    struct namespace *next;  
};


struct proc_namespace {
    struct namespace *mounts;
};


int bind(const char *new, const char *old, int flags);
int mount(const char *srv, const char *old, int flags, const char *spec);
int unbind(const char *path);

struct namespace* create_namespace(const char *old, const char *new, int flags);
void init_process_namespace(struct proc_namespace *ns);

#endif 