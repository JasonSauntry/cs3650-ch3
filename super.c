#include "super.h"

#include "string.h"

#include "pages.h"

super_block* get_super() {
	return pages_get_page(SUPER_PAGE);
}

void init_super() {
	super_block* super = get_super();
	init_bitmaps(&super->maps);
	super->maps.block_bitmap.bits[0] = 1; // Super block reserved.

	int most_recent_version = 0;
	memset(super->versions, 0, sizeof(version) * VERSIONS_KEPT);
}

int get_root_inum() {
	super_block* super = get_super();
	int index = super->most_recent_version;
	return super->versions[index % VERSIONS_KEPT].root;
}

void super_rollback(int version) {
	super_block* super = get_super();
	super->most_recent_version = version;
	for (int ii = 0; ii < VERSIONS_KEPT; ii++) {
		if (super->versions[ii].version_number == version) {
			// Then the version number is correct, reset base node
		}

	}		
}
