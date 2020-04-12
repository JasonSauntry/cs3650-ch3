#include "super.h"

#include "pages.h"

super_block* get_super() {
	return pages_get_page(SUPER_PAGE);
}

void init_super() {
	super_block* super = get_super();
	init_bitmaps(&super->maps);
	super->maps.block_bitmap.bits[0] = 1; // Super block reserved.
}

void super_rollback(int version) {
	super_block* super = get_super();
	for (int ii = 0; ii < VERSIONS_KEPT; ii++) {
		if (super->versions[ii] == version) {
			// Then the version number is correct
			memmove(&super->versions[ii], &super->versions[ii + 1], (VERSIONS_KEPT - ii - 1)*sizeof(*(super->versions)));
		}

	}		
}
