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
int    storage_stat(const char* path, struct stat* st, int version);
int    storage_read(const char* path, char* buf, size_t size, off_t offset, int version);
int    storage_write(const char* path, const char* buf, size_t size, off_t offset, int version);
int    storage_truncate(const char *path, off_t size, int version);
int    storage_mknod(const char* path, int mode, int dir, int version); 
int    storage_unlink(const char* path, int version);
int    storage_link(const char *from, const char *to, int version);
int    storage_rename(const char *from, const char *to, int version);
int    storage_set_time(const char* path, const struct timespec ts[2], int version);
int    storage_set_mode(const char* path, const int mode, int version);
slist* storage_list(const char* path, int version);
int    storage_symlink(const char* dest, const char* name, int version);
int    storage_readlink(const char* path, char* buf, size_t size, int version);

// Get AND increment.
int storage_copy_file(int inum, int version);
int storage_copy_dir(int inum, int version);
int storage_copy_root(const char* trigger);

#endif
