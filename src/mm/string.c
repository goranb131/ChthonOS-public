#include "string.h"

void* memset(void* s, int c, size_t n) {
    unsigned char* p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

size_t strlen(const char* s) {
    const char* p = s;
    while (*p) p++;
    return p - s;
}

void* memcpy(void* dst, const void* src, size_t n) {
    char* d = dst;
    const char* s = src;
    while (n--) *d++ = *s++;
    return dst;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

char *strcpy(char *dest, const char *src) {
    char *d = dest;
    while ((*d++ = *src++) != '\0');
    return dest;
}

char *strcat(char *dest, const char *src) {
    char *d = dest;
    while (*d) d++;  
    while ((*d++ = *src++) != '\0');  
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    return dest;
}

char* strrchr(const char* str, int c) {
    char* last = NULL;
    
    while (*str) {
        if (*str == (char)c) {
            last = (char*)str;
        }
        str++;
    }
    
    if (c == '\0') {
        return (char*)str;
    }
    
    return last;
}

char *strstr(const char *haystack, const char *needle) {
    if (!*needle) {
        return (char*)haystack;
    }
    
    while (*haystack) {
        const char *h = haystack;
        const char *n = needle;
        
        while (*h && *n && *h == *n) {
            h++;
            n++;
        }
        
        if (!*n) {
            return (char*)haystack;
        }
        
        haystack++;
    }
    
    return NULL;
}

size_t strspn(const char *str, const char *accept) {
    const char *s;
    size_t count = 0;
    
    while (*str) {
        for (s = accept; *s; s++) {
            if (*str == *s) {
                break;
            }
        }
        if (*s == '\0') {
            return count;
        }
        str++;
        count++;
    }
    return count;
}

char *strpbrk(const char *str, const char *accept) {
    while (*str) {
        const char *a = accept;
        while (*a) {
            if (*str == *a) {
                return (char *)str;
            }
            a++;
        }
        str++;
    }
    return NULL;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
    char *token;
    
    if (str == NULL) {
        str = *saveptr;
    }

    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }

    token = str;
    str = strpbrk(str, delim);
    if (str == NULL) {
        *saveptr = token + strlen(token);
    } else {
        *str = '\0';
        *saveptr = str + 1;
    }
    
    return token;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    
    
    if (d > s && d < s + n) {
        d += n;
        s += n;
        while (n--) {
            *--d = *--s;
        }
    } else {
        
        while (n--) {
            *d++ = *s++;
        }
    }
    
    return dest;
} 