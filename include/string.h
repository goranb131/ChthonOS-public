#ifndef STRING_H
#define STRING_H

#include <stddef.h>

void* memset(void* s, int c, size_t n);
int strcmp(const char* s1, const char* s2);
size_t strlen(const char* s);
void* memcpy(void* dst, const void* src, size_t n);
int strncmp(const char* s1, const char* s2, size_t n);
char *strcpy(char *dest, const char *src);
char *strcat(char *dest, const char *src);
char *strncpy(char *dest, const char *src, size_t n);
char* strrchr(const char* str, int c);
char *strstr(const char *haystack, const char *needle);
char *strtok_r(char *str, const char *delim, char **saveptr);
size_t strspn(const char *str, const char *accept);
char *strpbrk(const char *str, const char *accept);
void *memmove(void *dest, const void *src, size_t n);

#endif 