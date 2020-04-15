#include "garbage.h"

#include "assert.h"

#include "bitmaps.h"
#include "super.h"
#include "inode.h"
#include "directory.h"
#include "util.h"

// Given a node, mark it's pages as used.
void mark_pages(bitmaps* b, inode* node) {
	int page;
	if (node->directory) {
		page = div_up(node->size , (ENT_SIZE * ENT_PER_PAGE));
	} else {
		page = div_up(node->size, PAGE_SIZE);
	}

	for (int i = 0; i < page; i++) {
		int pnum = node->pages[i];
		b->block_bitmap.bits[pnum] = 1;
	//	printf("garbage mark \t page %d\n", i);
	}
}

// Given a node, mark it and it's pages as used.
void mark_node(bitmaps* b, int inum) {
	inode* node = get_inode(inum);
	b->inode_map.bits[inum] = 1;
	printf("garbage mark node %d\n", inum);
	mark_pages(b, node);
}

// Given a dir, mark it's children as used.
void mark_children(bitmaps* b, int inum) {
	inode* node = get_inode(inum);
	int count = node->size / ENT_SIZE;
	for (int i = 0; i < count; i++) {
		dir_ent* ent = directory_get(inum, i);
		int cnum = ent->inode_num;
		if (cnum != -1) {
			inode* c = get_inode(cnum);
			mark_node(b, cnum);
			if (c->directory) {
				mark_children(b, cnum);
			}
		}
	}
}

void mark_roots(bitmaps* b) {
	super_block* super = get_super();
	int max_versions = super->most_recent_version;
	version* versions = super->versions;
	for (int i = super->most_recent_version; i >= 0; i--) {
		int index = i % VERSIONS_KEPT;
		int named_version = versions[index].version_number;
		if (named_version != i) {
			break;
		}
		
		mark_node(b, versions[index].root);
		mark_children(b, versions[index].root);
	}
}


void mark_reserved(bitmaps* b) {
	for (int i = 0; i < INODE_AFTER_PAGE; i++) {
		b->block_bitmap.bits[i] = 1;
	}
}

/*
 * Mark used as 1, unused not changed.
 */
void mark_used(bitmaps* b) {
	init_bitmaps(b);
	mark_reserved(b);

	mark_roots(b);
}

void delete_unused(bitmaps* b) {
	bitmaps* global = &get_super()->maps;
	for (int i = 0; i < BITMAP_LEN; i++) {
		if (!b->block_bitmap.bits[i]) {
			free_page(i);
		}
		assert(b->block_bitmap.bits[i] == global->block_bitmap.bits[i]);

		if (!b->inode_map.bits[i]) {
			free_inode(i);
		}
		assert(b->inode_map.bits[i] == global->inode_map.bits[i]);
	}
}

void collect() {
	bitmaps maps;

	mark_used(&maps);

	delete_unused(&maps);
}
