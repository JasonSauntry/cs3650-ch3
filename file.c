#include "file.h"

#include <string.h>
#include <assert.h>

#include "util.h"

void get_data(inode* node, void* buffer, size_t size, size_t offset) {
	int start_page = offset / PAGE_SIZE;
	int first_index_on_page = start_page * PAGE_SIZE;
	int index_on_page = offset - first_index_on_page;
	int first_index_on_next_page = first_index_on_page + PAGE_SIZE;
	int last_index = offset + size;
	int single_page = last_index < first_index_on_next_page;

	void* start_page_addr = pages_get_page(node->pages[start_page]);
	void* source = start_page_addr + index_on_page;

	int read_len;
	if (single_page) {
		read_len = size;
	} else {
		read_len = PAGE_SIZE - index_on_page;
		get_data(node, buffer + read_len, size - read_len, offset + read_len);
	}

	memcpy(buffer, source, read_len);
}

void put_data(inode* node, void* buffer, size_t size, size_t offset) {
	if (size == 0) {
		return;
	}
	int start_page = offset / PAGE_SIZE;
	int first_index_on_page = start_page * PAGE_SIZE;
	int index_on_page = offset - first_index_on_page;
	int first_index_on_next_page = first_index_on_page + PAGE_SIZE;
	int last_index = offset + size;
	int single_page = last_index < first_index_on_next_page;

	void* start_page_addr = pages_get_page(node->pages[start_page]);
	void* dest = start_page_addr + index_on_page;
	void* source = buffer;

	int read_len;
	if (single_page) {
		read_len = size;
	} else {
		read_len = PAGE_SIZE - index_on_page;
		put_data(node, buffer + read_len, size - read_len, offset + read_len);
	}

	// memcpy(buffer, source, read_len);
	memcpy(dest, source, read_len);
}

void shrink(inode* node, size_t size) {
	node->size = size;
	int pages_needed = div_up(size, PAGE_SIZE);
	int final_page_index = pages_needed - 1;
	int first_offset_on_final_page = pages_needed * PAGE_SIZE;
	int first_unused_index_on_final_page = size - first_offset_on_final_page;
	assert(final_page_index == -1 || node->pages[final_page_index]);
	void* final_page = pages_get_page(node->pages[final_page_index]);
	void* first_unused_pointer = final_page + first_unused_index_on_final_page;
	int unused_len = PAGE_SIZE - first_unused_index_on_final_page;

	// Free all unneeded pages.
	for (int i = pages_needed; node->pages[i]; i++) {
		// free_page(node->pages[i]);
	}

	// Set unused data in the last page to 0.
	if (final_page_index >= 0) {
		memset(first_unused_pointer, 0, unused_len);
	}
}

void grow(inode* node, size_t size) {
	size_t oldsize = node->size;
	node->size = size;

	int mapped_pages = div_up(oldsize, PAGE_SIZE);
	if (oldsize % PAGE_SIZE != 0) {
		void* last_old_mapped_page = pages_get_page(node->pages[mapped_pages - 1]);
		int garbage_offset = oldsize * PAGE_SIZE;

		// Set garbage to 0.
		memset(last_old_mapped_page + garbage_offset, 0, PAGE_SIZE - garbage_offset);
	}

	size_t mapped_bytes = mapped_pages * PAGE_SIZE;
	// Map new pages.
	for (int i = mapped_pages; mapped_bytes < size; i++) {
		int newpage = map_page();
		node->pages[i] = newpage;
		mapped_bytes += PAGE_SIZE;
	}
}
