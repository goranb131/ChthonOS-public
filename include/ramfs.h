#ifndef RAMFS_H
#define RAMFS_H

#include "vfs.h"

extern struct filesystem_type ramfs_fs_type;

void ramfs_init(void);
int ramfs_create_file(const char *path, const char *content);
int ramfs_read_file(const char *path, char *out, int max_len);

#endif
