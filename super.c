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
