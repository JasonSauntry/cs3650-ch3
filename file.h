#ifndef FILE_H
#define FILE_H

#include "inode.h"

// Invariant: We only deal with most recent version.

/**
 * Get data from the file. Assumes input is valid.
 *
 * Sizes are bytes.
 */
void get_data(inode* node, void* buffer, size_t size, size_t offset);

/**
 * Put data into the file. Assumes input is valid. Assumes file is sufficiently
 * large.
 *
 * Sizes are bytes.
 */
void put_data(inode* node, void* buffer, size_t size, size_t offset);

/**
 * Shrink the file to the given size. Input must be valid. New size must be no
 * larger than existing size.
 * 
 * Unused data is set to 0.
 */
void shrink(inode* node, size_t size);

/**
 * Grow the file to the given size. Input must be valid. New size must be no
 * smaller than existing size.
 *
 * New data is set to 0.
 */
void grow(inode* node, size_t size);

#endif
