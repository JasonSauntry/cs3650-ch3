#ifndef SUPER_H
#define SUPER_H

#include "bitmaps.h"

#define SUPER_PAGE 0

typedef struct super_block {
	int unused; // I don't know why I need this, but it breaks without it. 
			// TODO maybe I should fix that.
	int root_inode;
	bitmaps maps;
} super_block;

super_block* get_super();

void init_super();

#endif
