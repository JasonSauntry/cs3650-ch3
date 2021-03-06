
#include <stdint.h>
#include <assert.h>

#include "pages.h"
#include "inode.h"
#include "util.h"
#include "super.h"

inode*
get_inode(int inum)
{
	int page_after_start = inum / INODES_PER_PAGE;
	int pagenum = page_after_start + 1;
	int first_on_page = page_after_start * INODES_PER_PAGE;
	int offset_on_page = inum - first_on_page;

	inode* page = pages_get_page(pagenum);

	inode* ours = &page[offset_on_page];
	
	return ours;
}

int
alloc_inode()
{
	bitmap* inode_bitmap = &(get_super()->maps.inode_map);
	int i;
	for (i = 0; i < BITMAP_LEN && inode_bitmap->bits[i]; i++);
	if (i >= BITMAP_LEN) {
		puts("+ Insufficient inodes");
		return -1;
	} else {
		inode_bitmap->bits[i] = 1;
		inode* node = get_inode(i);
		node->refs = 0;
		
		for (int i = 0; i < PAGE_ARRAY_SIZE; i++) {
			node->pages[i] = 0;
		}
		return i;
	}

}

void
free_inode(int inum)
{
    inode* node = get_inode(inum);

    // memset(node, 0, sizeof(inode));
    get_super()->maps.inode_map.bits[inum] = 0;
}

void
print_inode(inode* node)
{
    if (node) {
        printf("node{mode: %04o, size: %d}\n",
               node->mode, node->size);
    }
    else {
        printf("node{null}\n");
    }
}

void alloc_inode_pages() {
	bitmap* pagemap = &get_super()->maps.block_bitmap;
	for (int i = INODE_START_PAGE; i < INODE_AFTER_PAGE; i++) {
		pagemap->bits[i] = 1;
	}
}

int inode_add_ref(inode* node, int dirnode) {
	assert(dirnode >= 0);
	node->refs++;

	return 0;
}

int inode_del_ref(inode* node, int dirnode) {
	node->refs--;
	return 0;
}

