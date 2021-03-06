#ifndef NUFS_STORAGE_H
#define NUFS_STORAGE_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#include "slist.h"

#define DEFAULT_ROOT_INODE 0
#define DEFAULT_SYMLINK_MODE SLNMODE | 0644

void   storage_init(const char* path, int create);
int    storage_stat(const char* path, struct stat* st);
int    storage_read(const char* path, char* buf, size_t size, off_t offset);
int    storage_write(const char* path, const char* buf, size_t size, off_t offset);
int    storage_truncate(const char *path, off_t size);
int    storage_mknod(const char* path, int mode, int dir); 
int    storage_unlink(const char* path);
int    storage_link(const char *from, const char *to);
int    storage_rename(const char *from, const char *to);
int    storage_set_time(const char* path, const struct timespec ts[2]);
int    storage_set_mode(const char* path, const int mode);
slist* storage_list(const char* path);
int    storage_symlink(const char* dest, const char* name);
int    storage_readlink(const char* path, char* buf, size_t size);

void trace_path(const char* path);

// Get AND increment.
int storage_copy_file(int inum);
int storage_copy_dir(int inum);
void storage_copy_root(const char* trigger);

#endif
