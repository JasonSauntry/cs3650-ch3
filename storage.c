
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <alloca.h>
#include <string.h>
#include <libgen.h>
#include <bsd/string.h>
#include <stdint.h>

#include "storage.h"
#include "slist.h"
#include "util.h"
#include "pages.h"
#include "inode.h"
#include "directory.h"
#include "bitmaps.h"
#include "super.h"
#include "file.h"
#include "svec.h"

void
storage_init(const char* path, int create)
{
	//printf("storage_init(%s, %d);\n", path, create);
	pages_init(path, create);
	if (create) {
		init_super(get_super());
		// Since the filesystem is fresh, we can assume 0 is available.
		alloc_inode_pages();
		directory_init(DEFAULT_ROOT_INODE, 1, 0);
	}
}

int
storage_stat(const char* path, struct stat* st, int version)
{
	printf("+ storage_stat(%s)\n", path);
	int inum = tree_lookup(path);
	if (inum < 0) {
		return inum;
	}

	inode* node = get_inode(inum);
	node->last_access = now();
	printf("+ storage_stat(%s); inode %d\n", path, inum);
	print_inode(node);

	memset(st, 0, sizeof(struct stat));
	st->st_ino = inum;
	st->st_mode  = node->mode;
	st->st_nlink = node->refs;
	st->st_uid	 = getuid();
	st->st_size  = node->size;
	st->st_blksize = PAGE_SIZE;
	st->st_atim = node->last_access;
	st->st_mtim = node->last_modified;
	return 0;
}

int
storage_read(const char* path, char* buf, size_t size, off_t offset, int version)
{
	int inum = tree_lookup(path);
	if (inum < 0) {
		return inum;
	}
	inode* node = get_inode(inum);
	for (int i = 0; i < MAX_HARD_LINKS; i++) {
		printf("In link %d:\t%d\n", i, node->in_links[i]);
	}
	node->last_access = now();
	printf("+ storage_read(%s); inode %d\n", path, inum);
	print_inode(node);

	if (offset >= node->size) {
		return 0;
	}

	if (offset + size >= node->size) {
		size = node->size - offset;
	}

	size_t size_bytes = size * sizeof(char);
	size_t offset_bytes = offset * sizeof(char);

	get_data(node, buf, size_bytes, offset_bytes);

	return size;
}

int
storage_write(const char* path, const char* buf, size_t size, off_t offset, int version)
{
	int inum = tree_lookup(path);
	if (inum < 0) {
		return inum;
	}


	int trv = storage_truncate(path, offset + size, version);
	if (trv < 0) {
		return trv;
	}

	inode* node = get_inode(inum);
	printf("+ storage_write inum: %d\n", inum);
	if (node->version < version) {
		puts("====== :( VERSION ======");
	}

	size_t size_bytes = size * sizeof(char);
	size_t offset_bytes = offset * sizeof(char);

	put_data(node, (void*) buf, size_bytes, offset_bytes);

	node->last_modified = node->last_access = now();

	return size;
}

int
storage_truncate(const char *path, off_t newsize, int version)
{
	int inum = tree_lookup(path);
	if (inum < 0) {
		return inum;
	}

	inode* node = get_inode(inum);
	if (node->version < version) {
		puts("====== :( VERSION ======");
	}
	off_t oldsize = node->size;
	if (newsize == oldsize) { }
	else if (newsize < oldsize) {
		// Make it smaller.
		shrink(node, newsize);
	} else {
		// Make it bigger.
		grow(node, newsize);
	}

	node->last_modified = node->last_access = now();
	return 0;
}

// Get the parent of a file.
int parent_inode(const char* cpath) {
	char* path = alloca(strlen(cpath));
	strcpy(path, cpath);
	char* last_dir_pointer = strrchr(path, '/');
	*last_dir_pointer = 0;

	if (strlen(path) == 0) {
		return get_root_inum();
	} else { 
		return tree_lookup(path);
	}
}

int
storage_mknod(const char* path, int mode, int dir, int version)
{
	char* tmp1 = alloca(strlen(path));
	char* tmp2 = alloca(strlen(path));
	strcpy(tmp1, path);
	strcpy(tmp2, path);

	const char* name = strrchr(path, '/') + 1;

	if (directory_lookup(parent_inode(path), name) != -ENOENT) {
		printf("mknod fail: already exist\n");
		return -EEXIST;
	} // TODO move

	int inum = alloc_inode(version);


	inode* node = get_inode(inum);
	node->mode = mode;
	node->size = 0;
	node->directory = dir;
	node->last_modified = node->last_access = now();
	if (dir) {
		directory_init(inum, 0, version);
	}

	printf("+ mknod create %s [%04o] - #%d\n", path, mode, inum);

	int parent = parent_inode(path);
	if (parent < 0) {
		puts("mknod fail: directory doesn't exist\n");
		return -ENOENT;
	}

	return directory_put(parent, name, inum, version);
}

slist*
storage_list(const char* path, int version)
{
	int dirnode = tree_lookup(path);
	return directory_list(dirnode);
}

int
storage_unlink(const char* path, int version)
{
	int parent_inum = parent_inode(path);
	const char* name = strrchr(path, '/') + 1;
	return directory_delete(parent_inum, name, version);
}

// The arguments meant the exact opposite of what I thought they did. So I
// swapped them.
int
storage_link(const char* to, const char* from, int version)
{
	int tarinum = (tree_lookup(to));
	int from_parent = (parent_inode(from));

	if (tarinum < 0) {
		return tarinum;
	} else if (from_parent < 0) {
		return from_parent;
	}

	char* from_name = strrchr(from, '/') + 1;

	int rv = directory_put(from_parent, from_name, tarinum, version);
	if (rv < 0) {
		return rv;
	}
	
	return 0;
}

int
storage_rename(const char* from, const char* to, int version)
{
	// TODO what if it's a dir?
	int rv = storage_link(from, to, version);
	if (rv < 0) {
		return rv;
	}

	rv = storage_unlink(from, version);
	if (rv < 0) {
		return rv;
	}
	
	return rv;
}

int storage_set_mode(const char* path, const int mode, int version) {
	int inum = tree_lookup(path);
	if (inum < 0) {
		return inum;
	}
	inode* node = get_inode(inum);
	if (node->version < version) {
		puts("====== :( VERSION ======");
	}

	node->mode = mode;
	if (node->directory) {
		node->mode = DIRMODE | mode;
	}
	if (node->link) {
		node->mode = SLNMODE | mode;
	}
	return 0;
}

int
storage_set_time(const char* path, const struct timespec ts[2], int version)
{
	int inum = tree_lookup(path);
	if (inum < 0) {
		return inum;
	}

	inode* node = get_inode(inum);
	if (node->version < version) {
		puts("====== :( VERSION ======");
	}
	node->last_access = ts[0];
	node->last_modified = ts[0];

	return 0;
}

bitmaps* get_bitmaps() {
	return &get_super()->maps;
}

int storage_symlink(const char* dest, const char* name, int version) {
	int parent = parent_inode(name);
	if (parent < 0) {
		return parent;
	}

	int rv = storage_mknod(name, DEFAULT_SYMLINK_MODE, 0, version);
	if (rv < 0) {
		return rv;
	}

	long len = strlen(dest) + 1; // Include the null-terminator!
	rv = storage_truncate(name, len, version);
	if (rv < 0) {
		return rv;
	}

	rv = storage_write(name, dest, len, 0, version);
	if (rv < 0) {
		return rv;
	}
	return 0;
}

int storage_readlink(const char* path, char* buf, size_t size, int version) {
	int rv = storage_read(path, buf, size, 0, version);
	if (rv < 0) {
		return rv;
	}
	return 0;
}

int get_most_recent_inum(int inum) {
	inode* node = get_inode(inum);
	if (node->next) {
		return get_most_recent_inum(node->next);
	} else {
		return inum;
	}
}

int trace_path_helper(const char* path) {
	int root = get_root_inum();
	int inum;
	if (streq(path, "")) {
		inum = root;
	} else {
		inum = tree_lookup(path);
	}

	if (inum == root) {
		printf("%d", inum);
	} else {
		char* abbreviated = strdup(path);
		char* last_sep = strrchr(abbreviated, '/');
		*last_sep = 0;

		int pnum = trace_path_helper(abbreviated);
		inode* node = get_inode(inum);
		// inode* parent = get_inode(pnum);

		// int check = 0;
		// for (int i = 0; i < MAX_HARD_LINKS; i++) {
		// 	if (node->in_links[i] != -1 && 
		// 			get_most_recent_inum(node->in_links[i]) == (pnum)) {
		// 		check = 1;
		// 	}
		// }
		// assert(check);

		free(abbreviated);
		printf(" --> %d", inum);
	}
	return inum;
}

void trace_path(const char* path) {
	int rv = tree_lookup(path);
	if (rv > 0) {
		printf("Path for %s: ", path);
		trace_path_helper(path);
		printf("\n");
	} else {
		printf("DNE: %s", path);
	}
}

void get_parent_inodes(int root, int searching_for, svec* vec) {
	inode* node = get_inode(root);
	assert(node->directory);
	int items = node->size / ENT_SIZE;

	for (int i = 0; i < items; i++) {
		dir_ent* ent = directory_get(root, i);
		if (ent->inode_num != -1) {
			inode* ent_node = get_inode(ent->inode_num);

			if (ent->inode_num == searching_for) {
				// Found a parent.
				if (!svec_contains(vec, root)) {
					svec_push_back(vec, root);
				}
			}

			if (ent_node->directory) {
				get_parent_inodes(ent->inode_num, searching_for, vec);
			}
		}
	}
}

// Copy this and modify (for now) all parents.
int storage_copy_file(int old_inum, int version) {
	inode* old_node = get_inode(old_inum);

	int new_inum = alloc_inode(version);

	inode* new_node = get_inode(new_inum); 
	memcpy(new_node, old_node, sizeof(inode)); 

	new_node->version = version; 
	new_node->next = 0; 
	old_node->next = new_inum; 

	printf("+ cpy file %d to %d\n", old_inum, new_inum);

	svec* parents = make_svec();
	get_parent_inodes(get_root_inum(), old_inum, parents);

	for (int i = 0; i < parents->size; i++) {
		int parent_node = svec_get(parents, i);
		assert(parent_node != -1);

		parent_node = storage_copy_dir(parent_node, version);

		// Update parent's link.
		directory_replace_ref(parent_node, old_inum, new_inum);

		// Update my link.
		new_node->in_links[i] = parent_node;
	}

	free_svec(parents);

	// Now copy data.
	if (1) {
		// TODO don't do this if unneccesary.
		for (int i = 0; i < PAGE_ARRAY_SIZE && new_node->pages[i]; i++) {
			new_node->pages[i] = pages_cpy(new_node->pages[i]);
		}

	}

	return new_inum;
}

int storage_copy_dir(int old_inum, int version) {
	inode* old = get_inode(old_inum);
	// if (old->version >= version) {
	// 	return old_inum;
	// }
	// Check if I'm root.
	super_block* super = get_super();
	int root = get_root_inum();
	printf("+ copy dir %d -> looking for root %d\n", old_inum, root);
	// if (root == old_inum) {
	if (old->refs == 0) {
		puts("I am root. ¯\\_(ツ)_/¯");

		return root;
	} else {
		return storage_copy_file(old_inum, version);
	}

}

int storage_copy_root(const char* trigger) {
	super_block* super = get_super();
	int* version = &super->most_recent_version;

	int new_rnum = storage_copy_file(get_root_inum(), *version + 1);
	(*version)++;
	int v = *version % VERSIONS_KEPT;
	super->versions[v].root = new_rnum;
	strlcpy(super->versions[v].trigger, trigger, DESC_LEN);
	super->versions[v].version_number = *version;
	
	return *version;
}
