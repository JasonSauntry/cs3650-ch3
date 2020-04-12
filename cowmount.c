#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>
#include <bsd/string.h>
#include <assert.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "storage.h"
#include "slist.h"
#include "util.h"

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
	printf("%s...\n", "access");
	int rv = 0;
	printf("access(%s, %04o) -> %d\n", path, mask, rv);
	return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
	printf("%s...\n", "getattr");
	int rv = storage_stat(path, st);
	printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
	return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
	printf("%s...\n", "readdir");
	const int buflen = 128;
	struct stat st;
	char item_path[buflen];
	int rv;

	rv = storage_stat(path, &st);
	assert(rv == 0);

	filler(buf, ".", &st, 0);

	strcpy(item_path, path);
	int pathlen = strlen(item_path);

	slist* items = storage_list(path);
	for (slist* xs = items; xs != 0; xs = xs->next) {
		printf("+ looking at path: '%s'\n", xs->data);

		item_path[pathlen] = '/';
		int nrot = pathlen == 1 ? 0 : 1;
		strlcpy(item_path + pathlen + nrot, xs->data, buflen - pathlen - 1);

		rv = storage_stat(item_path, &st);
		assert(rv == 0);
		filler(buf, xs->data, &st, 0);
	}
	s_free(items);

	printf("readdir(%s) -> %d\n", path, rv);
	return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	printf("%s...\n", "mknod");
	int* version = storage_get_inc_version();
	int rv = storage_mknod(path, mode, 0, *version);
	printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
	return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
	printf("%s...\n", "mkdir");
	int* version = storage_get_inc_version();
	int rv = storage_mknod(path, mode, 1, *version);
	printf("mkdir(%s) -> %d\n", path, rv);
	return rv;
}

int
nufs_unlink(const char *path)
{
	printf("%s...\n", "unlink");
	int* version = storage_get_inc_version();
	int rv = storage_unlink(path, *version);
	printf("unlink(%s) -> %d\n", path, rv);
	return rv;
}

int
nufs_link(const char *from, const char *to)
{
	printf("%s...\n", "link");
	int* version = storage_get_inc_version();
	int rv = storage_link(from, to, *version);
	printf("link(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_rmdir(const char *path)
{
	printf("%s...\n", "rmdir");
	int* version = storage_get_inc_version();
	int rv = storage_unlink(path, *version);
	printf("rmdir(%s) -> %d\n", path, rv);
	return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
	printf("%s...\n", "rename");
	int* version = storage_get_inc_version();
	int rv = storage_rename(from, to, *version);
	printf("rename(%s => %s) -> %d\n", from, to, rv);
	return rv;
}

int
nufs_chmod(const char *path, mode_t mode)
{
	printf("%s...\n", "chmod");
	int* version = storage_get_inc_version();
	int rv = storage_set_mode(path, mode, *version);
	printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
	return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
	printf("%s...\n", "truncate");
	int* version = storage_get_inc_version();
	int rv = storage_truncate(path, size, *version);
	printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
	return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
	printf("%s...\n", "open");
	int rv = nufs_access(path, 0);
	printf("open(%s) -> %d\n", path, rv);
	return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("%s...\n", "read");
	int rv = storage_read(path, buf, size, offset);
	printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
	return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("%s...\n", "write");
	int* version = storage_get_inc_version();
	int rv = storage_write(path, buf, size, offset, *version);
	printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
	return rv;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
	printf("%s...\n", "utimens");
	int* version = storage_get_inc_version();
	int rv = storage_set_time(path, ts, *version);
	printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
		   path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	  return rv;
}

int nufs_readlink(const char* path, char* buf, size_t size) {
	printf("%s...\n", "readlink");
	int rv = storage_readlink(path, buf, size);
	printf("readlink(%s, %ld) -> %d\n ==> %s\n", path, size, rv, buf);
	return rv;
}

int nufs_symlink(const char* to, const char* from) {
	printf("%s...\n", "symlink");
	int* version = storage_get_inc_version();
	int rv = storage_symlink(to, from, *version);
	printf("symlink(%s, %s) -> %d\n", to, from, rv);
	return rv;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
		   unsigned int flags, void* data)
{
	printf("%s...\n", "ioctl");
	int rv = -1;
	printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
	return rv;
}

void
nufs_init_ops(struct fuse_operations* ops)
{
	memset(ops, 0, sizeof(struct fuse_operations));
	ops->access   = nufs_access;
	ops->getattr  = nufs_getattr;
	ops->readdir  = nufs_readdir;
	ops->mknod	  = nufs_mknod;
	ops->mkdir	  = nufs_mkdir;
	ops->link	  = nufs_link;
	ops->unlink   = nufs_unlink;
	ops->rmdir	  = nufs_rmdir;
	ops->rename   = nufs_rename;
	ops->chmod	  = nufs_chmod;
	ops->truncate = nufs_truncate;
	ops->open		= nufs_open;
	ops->read	  = nufs_read;
	ops->write	  = nufs_write;
	ops->utimens  = nufs_utimens;
	ops->ioctl	  = nufs_ioctl;
	ops->readlink = nufs_readlink;
	ops->symlink  = nufs_symlink;
};

struct fuse_operations nufs_ops;

int
main(int argc, char *argv[])
{
	assert(argc > 2 && argc < 6);
	storage_init(argv[--argc], 0);
	nufs_init_ops(&nufs_ops);
	return fuse_main(argc, argv, &nufs_ops, NULL);
}

