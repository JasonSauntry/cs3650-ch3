
#define _GNU_SOURCE
#include <string.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "directory.h"
#include "pages.h"
#include "slist.h"
#include "util.h"
#include "inode.h"
#include "super.h"

// Only called on init.
void
directory_init(int dirnode, int new, int version)
{
	inode* rn = get_inode(dirnode);
	get_super()->maps.inode_map.bits[dirnode] = 1;

	if (new) {
		// We are root.
		rn->mode = 0755;
		rn->last_access = now();
		rn->last_modified = now();
		rn->version = version;
	}
	for (int i = 0; i < MAX_HARD_LINKS; i++) {
		rn->in_links[i] = -1;
	}
	rn->mode = rn->mode | DIRMODE;
	rn->directory = 1;
	// Don't allocate any data pages yet.
	for (int i = 0; i < PAGE_ARRAY_SIZE; i++) {
		rn->pages[i] = 0;
	}
}

dir_ent*
directory_get(int dirnode, int ii)
{
	inode* node = get_inode(dirnode);
	node->last_access = now();

	int page_number = ii / ENT_PER_PAGE;
	int first_on_page = page_number * ENT_PER_PAGE;
	int index_on_page = ii - first_on_page;

	int page_pointer = node->pages[page_number];
	if (page_pointer) {
		dir_page* page_content = pages_get_page(node->pages[page_number]);

		dir_ent* entry = &page_content->files[index_on_page];

		return entry;
	} else {
		return 0;
	}
}

int
directory_lookup(int dirnode, const char* name)
{
	inode* node = get_inode(dirnode);
	node->last_access = now();
	int entries = node->size / ENT_SIZE;
	for (int ii = 0; ii < entries; ++ii) {
		dir_ent* ent = directory_get(dirnode, ii);
		if (ent->filename[0] && streq(ent->filename, name)) {
			return ii;
		}
	}

	return -ENOENT;

	puts("directory_lookup unreachable code reached");
	puts("very bad");
	abort();
}

int
tree_lookup(const char* path)
{
	assert(path[0] == '/');

	if (streq(path, "/")) {
		return get_root_inum();
	} else {
		// We have a parent directory.
		char* last_dir_pointer = strrchr(path, '/');
		int last_dir_sep = last_dir_pointer - path;

		char* abbreviated = strdup(path);
		abbreviated[last_dir_sep + 1] = 0;
		if (last_dir_sep != 0) {
			abbreviated[last_dir_sep] = 0;
		}
		char* relative_name = last_dir_pointer + 1;

		int parent_inum = tree_lookup(abbreviated);
		free(abbreviated);
		if (parent_inum < 0) {
			puts("tree_lookup error");
			printf("%d\n", parent_inum);
			return parent_inum;
		}

		inode* parent = get_inode(parent_inum);
		
		int out_index = directory_lookup(parent_inum, relative_name);
		if (out_index < 0) {
			puts("tree_lookup error");
			printf("%d\n", out_index);
			return out_index;
		}
		int out_node = directory_get(parent_inum, out_index)->inode_num;

		return out_node;
	}

}

int
directory_put(int dirnode, const char* name, int inum, int version)
{
	// TODO use empty slots if available.
	inode* node = get_inode(dirnode);
	if (node->version < version) {
		puts("====== :( VERSION ======");
	}
	node->last_access = node->last_modified = now();
	int cur_objs = node->size / ENT_SIZE;
	int first_empty;
	for (first_empty = 0; 
			(first_empty < cur_objs)
				&& directory_get(dirnode, first_empty)->filename[0]; 
			first_empty++);
	int index;
	if (first_empty < cur_objs) {
		index = first_empty;
	} else {
		int new_index = cur_objs;
		int new_page = cur_objs % ENT_PER_PAGE == 0;
		int page_number = cur_objs / ENT_PER_PAGE;
		int first_index_on_page = page_number * ENT_PER_PAGE;
		int index_on_page = new_index - first_index_on_page;

		if (new_page) {
			int page = map_page();
			node->pages[page_number] = page;
		}

		node->size += sizeof(dir_ent);
		index = new_index;
	}

	// dir_page* page = pages_get_page(node->pages[page_number]);

	dir_ent* entity = directory_get(dirnode, index);
	entity->inode_num = inum;
	strlcpy(entity->filename, name, NAME_LEN);

	inode* tarnode = get_inode(inum);
	int rv = inode_add_ref(tarnode, dirnode);
	if (rv < 0) {
		puts("directory_put inode_add_ref :(");
		return rv;
	}
	printf("+ dirent = '%s'\n", entity->filename);

	printf("+ directory_put(..., %s, %d) -> 0\n", name, inum);
	print_inode(node);

	return 0;
}

int
directory_delete(int dirnode, const char* name, int version)
{
	printf(" + directory_delete(%s)\n", name);

	int index = directory_lookup(dirnode, name);
	if (index < 0) {
		return index;
	}
	dir_ent* ent = directory_get(dirnode, index);
	inode* dir = get_inode(dirnode);
	if (dir->version < version) {
		puts("====== :( VERSION ======");
	}
	dir->last_access = dir->last_modified = now();
	inode* file_inode = get_inode(ent->inode_num);
	inode_del_ref(file_inode, dirnode);
	int dinum = ent->inode_num;
	ent->inode_num = -1;
	ent->filename[0] = 0;
	// if (file_inode->refs == 0) {
	// 	inode* node = file_inode;
	// 	assert(node->refs == 0);
	// 	free_inode(dinum);
	// }

	return 0;
}

int directory_replace_ref(int dirnode, int old_node, int new_node) {
	// Assume version is sufficiently high.
	assert(dirnode >= 0 && dirnode < INODE_COUNT);
	inode* dir = get_inode(dirnode);
	int entries = dir->size / ENT_SIZE;

	for (int i = 0; i < entries; i++) {
		dir_ent* ent = directory_get(dirnode, i);
		if (ent->inode_num == old_node) {
			ent->inode_num = new_node;
			printf("+ replace_ref %d with %d\n", old_node, new_node);
		}
		assert(directory_get(dirnode, i)->inode_num != old_node);
	}
	return 0;
}

slist*
directory_list(int dirnode)
{
	// printf("+ directory_list()\n");
	slist* ys = 0;

	inode* node = get_inode(dirnode);
	node->last_access = node->last_modified = now();
	int entries = node->size / ENT_SIZE;

	for (int ii = 0; ii < entries; ++ii) {
		char* ent = directory_get(dirnode, ii)->filename;
		if (ent[0]) {
			// printf(" - %d: %s [%d]\n", ii, ent, ii);
			ys = s_cons(ent, ys);
		}
	}

	return ys;
}

void
print_directory(int dd)
{
	printf("Contents:\n");
	slist* items = directory_list(dd);
	for (slist* xs = items; xs != 0; xs = xs->next) {
		printf("- %s\n", xs->data);
	}
	printf("(end of contents)\n");
	s_free(items);
}


