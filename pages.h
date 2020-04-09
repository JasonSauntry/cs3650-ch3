#ifndef PAGES_H
#define PAGES_H

#include <stdio.h>

#define PAGE_SIZE 4096

void pages_init(const char* path, int create);
void pages_free();
void* pages_get_page(int pnum);

/**
 * Find an unused page, allocate it, return its index.
 */
int map_page();

/**
 * Deallocate a used page.
 */
void free_page(int i);

#endif
