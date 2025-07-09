#include "kmalloc.h"

static char heap[1024 * 1024];  
static size_t heap_pos = 0;

void* kalloc(size_t size) {
    if (heap_pos + size > sizeof(heap)) {
        return NULL;
    }
    void* ptr = &heap[heap_pos];
    heap_pos += size;
    return ptr;
}

void kfree(void* ptr) {
    
    
    (void)ptr;
} 