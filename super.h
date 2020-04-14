#ifndef SUPER_H
#define SUPER_H

#include "bitmaps.h"

#define SUPER_PAGE 0

#define DESC_LEN 32
#define VERSIONS_KEPT 16

typedef struct version {
	int root;
	int version_number;
	char trigger[DESC_LEN];
	char path[DESC_LEN];
} version;

typedef struct super_block {
	int unused; // I don't know why I need this, but it breaks without it. 
			// TODO maybe I should fix that.
	// int root_inode;
	bitmaps maps;
	int most_recent_version;
	version versions[VERSIONS_KEPT]; // Index by mod-16.
	
} super_block;

super_block* get_super();

void init_super();

int get_root_inum();

void super_rollback(int version);

#endif
