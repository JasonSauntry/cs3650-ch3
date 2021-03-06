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
#include "directory.h"
#include "super.h"
#include "garbage.h"

int nufs_cpy_actually(const char* path, const char* trigger) {
	printf("Copying %s ...\n", path);
	trace_path(path);
	int old_inum = tree_lookup(path);
	collect();
	storage_copy_root(trigger);
	int inum = tree_lookup(path);
	int rv = storage_copy_dir(inum);
	int new_inum = tree_lookup(path);
	trace_path(path);
	printf("Copy %s %d --> %d == %d\n", path, old_inum, rv, new_inum);
	return rv;
}

int nufs_cpy(const char* path, const char* syscall) {

	// Description: "syscall /path/to/file"
	const int desc_size = DESC_LEN * sizeof(char);
	char* trigger = alloca(desc_size);
	strncpy(trigger, syscall, desc_size);
	strncat(trigger, " ", desc_size);
	strncat(trigger, path, desc_size);

	return nufs_cpy_actually(path, trigger);
}

int nufs_cpy_parent(const char* path, const char* syscall) {
	if (strlen(path) == 1) {
		puts("Root has no parent");
		return -1;
	}

	char* mutable_path = strdup(path);
	char* dir_sep = strrchr(mutable_path, '/');
	*dir_sep = 0;
	if (mutable_path == dir_sep) {
		mutable_path[0] = '/';
		mutable_path[1] = 0;
	}

	// Description: "syscall /path/to/file"
	const int desc_size = DESC_LEN * sizeof(char);
	char* trigger = alloca(desc_size);
	strncpy(trigger, syscall, desc_size);
	strncat(trigger, " ", desc_size);
	strncat(trigger, path, desc_size);

	int rv = nufs_cpy_actually(mutable_path, trigger);

	free(mutable_path);

	return rv;
}

// implementation for: man 2 access
// Checks if a file exists.
int
nufs_access(const char *path, int mask)
{
	int rv = 0; 
	printf("access(%s, %04o) -> %d\n", path, mask, rv);
	return rv;
}

// implementation for: man 2 stat
// gets an object's attributes (type, permissions, size, etc)
int
nufs_getattr(const char *path, struct stat *st)
{
	// int n = nufs_cpy(path);
	// if (n < 0) {
	// 	puts(":(");
	// 	return n;
	// }
	int rv = storage_stat(path, st);
	printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode, st->st_size);
	trace_path(path);
	return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int
nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
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
	trace_path(path);
	return 0;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
int
nufs_mknod(const char *path, mode_t mode, dev_t rdev)
{
	// Special.
	int n = nufs_cpy_parent(path, "mknod");
	if (n < 0) {
		puts(":(");
		return n;
	}
	int rv = storage_mknod(path, mode, 0);
	printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
	trace_path(path);
	return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int
nufs_mkdir(const char *path, mode_t mode)
{
	int n = nufs_cpy_parent(path, "mkdir");
	if (n < 0) {
		puts(":(");
		return n;
	}
	int rv = storage_mknod(path, mode, 1);
	printf("mkdir(%s) -> %d\n", path, rv);
	trace_path(path);
	return rv;
}

int
nufs_unlink(const char *path)
{
	int n = nufs_cpy(path, "unlink");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_unlink(path);
	printf("unlink(%s) -> %d\n", path, rv);
	trace_path(path);
	return rv;
}

int
nufs_link(const char *from, const char *to)
{
	int n = nufs_cpy(from, "link");
	if (n < 0) {
		puts(":(");
		return n;
	}
	n = nufs_cpy_parent(to, "link");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_link(from, to);
	printf("link(%s => %s) -> %d\n", from, to, rv);
	trace_path(from);
	trace_path(to);
	return rv;
}

int
nufs_rmdir(const char *path)
{
	int n = nufs_cpy(path, "rmdir");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_unlink(path);
	printf("rmdir(%s) -> %d\n", path, rv);
	trace_path(path);
	return rv;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int
nufs_rename(const char *from, const char *to)
{
	int n = nufs_cpy(from, "rename");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_rename(from, to);
	printf("rename(%s => %s) -> %d\n", from, to, rv);
	trace_path(from);
	trace_path(to);
	return rv;
}

int
nufs_chmod(const char *path, mode_t mode)
{
	int n = nufs_cpy(path, "chmod");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_set_mode(path, mode);
	printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
	trace_path(path);
	return rv;
}

int
nufs_truncate(const char *path, off_t size)
{
	int n = nufs_cpy(path, "truncate");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_truncate(path, size);
	printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
	trace_path(path);
	return rv;
}

// this is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
int
nufs_open(const char *path, struct fuse_file_info *fi)
{
	int rv = nufs_access(path, 0);
	printf("open(%s) -> %d\n", path, rv);
	trace_path(path);
	return rv;
}

// Actually read data
int
nufs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int rv = storage_read(path, buf, size, offset);
	printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
	trace_path(path);
	return rv;
}

// Actually write data
int
nufs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	int n = nufs_cpy(path, "write");
	if (n < 0) {
		puts(":(");
		return n;
	}
	
	int rv = storage_write(path, buf, size, offset);
	printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
	trace_path(path);
	return rv;
}

// Update the timestamps on a file or directory.
int
nufs_utimens(const char* path, const struct timespec ts[2])
{
	int n = nufs_cpy(path, "utimens");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_set_time(path, ts);
	printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n",
		   path, ts[0].tv_sec, ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
	trace_path(path);
	  return rv;
}

int nufs_readlink(const char* path, char* buf, size_t size) {
	int rv = storage_readlink(path, buf, size);
	printf("readlink(%s, %ld) -> %d\n ==> %s\n", path, size, rv, buf);
	trace_path(path);
	return rv;
}

int nufs_symlink(const char* to, const char* from) {
	// Special
	printf("symlink:\nto: %s\nfrom: %s\n", to, from);
	int n = nufs_cpy_parent(from, "symlink");
	if (n < 0) {
		puts(":(");
		return n;
	}

	int rv = storage_symlink(to, from);
	printf("symlink(%s, %s) -> %d\n", to, from, rv);
	trace_path(to);
	trace_path(from);
	return rv;
}

// Extended operations
int
nufs_ioctl(const char* path, int cmd, void* arg, struct fuse_file_info* fi,
		   unsigned int flags, void* data)
{
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

