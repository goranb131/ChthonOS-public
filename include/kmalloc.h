#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>

void* kalloc(size_t size);
void kfree(void* ptr);

#endif 