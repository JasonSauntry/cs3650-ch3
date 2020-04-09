#include <string.h>

#include "bitmaps.h"

int first_empty(bitmap* map) {
	for (int i = 0; i < BITMAP_LEN; i++) {
		if (!map->bits[i]) {
			return i;
		}
	}
	return -1;
}

void init_bitmap(bitmap* map) {
	memset(map, 0, sizeof(bitmap));
}

void init_bitmaps(bitmaps* map) {
	init_bitmap(&map->block_bitmap);
	init_bitmap(&map->inode_map);
}
