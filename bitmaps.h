#ifndef BITMAPS_H
#define BITMAPS_H

#define BITMAP_LEN 256

typedef struct bitmap {
	unsigned short int bits[BITMAP_LEN]; // We'll allocate a whole byte per bit, for convenience.
} bitmap;

typedef struct bitmaps {
	bitmap inode_map;
	bitmap block_bitmap;
} bitmaps;

/** 
 * Get the index of the first unused bit in the bitmap. -1 on failure.
 */
int first_empty(bitmap* map);

/**
 * Initialize to 0;
 */
void init_bitmap(bitmap* map);

/**
 * Initialize to 0;
 */
void init_bitmaps(bitmaps* maps);

#endif
