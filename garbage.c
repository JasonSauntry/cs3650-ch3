#include "garbage.h"

#include "assert.h"

#include "bitmaps.h"
#include "super.h"
#include "inode.h"
#include "directory.h"

void mark_pages(bitmaps* b, inode* node) {
	int item_count = node->size / ENT_SIZE;
	int page_count = item_count / ENT_PER_PAGE;
	for (int i = 0; i < page_count; i++) {
		int pagenum = node->pages[i];
		b->block_bitmap.bits[pagenum] = 1;
	}
}

// Invariant: inum must refer to a directory.
void mark_used_from_directory(bitmaps* b, int inum) {
	inode* node = get_inode(inum);
	assert(node->directory);
	
	// Pages
	mark_pages(b, node);
	
	// Children
	int item_count = node->size / ENT_SIZE;
	for (int i = 0; i < item_count; i++) {
		dir_ent* ent = directory_get(inum, i);
		int childnum = ent->inode_num;
		if (!b->inode_map.bits[childnum]) {
			b->inode_map.bits[childnum] = 1;
			inode* childnode = get_inode(childnum);
			if (childnode->directory) {
				mark_used_from_directory(b, childnum);
			}
		}
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

	super_block* super = get_super();
	for (int i = 0; i < VERSIONS_KEPT && i <= super->most_recent_version; i++) {
		int rnum = super->versions[i].root;
		mark_pages(b, get_inode(rnum));
		b->inode_map.bits[rnum] = 1;
		mark_used_from_directory(b, rnum);
	}
}

void delete_unused(bitmaps* b) {
	bitmaps* global = &get_super()->maps;
	for (int i = 0; i < BITMAP_LEN; i++) {
		if (!b->block_bitmap.bits[i] && global->block_bitmap.bits[i]) {
			free_page(i);
		}

		if (!b->inode_map.bits[i] && global->inode_map.bits[i]) {
			free_inode(i);
		}
	}
}

void collect() {
	bitmaps maps;

	mark_used(&maps);

	delete_unused(&maps);
}
