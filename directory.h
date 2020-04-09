#ifndef DIRECTORY_H
#define DIRECTORY_H

#include <bsd/string.h>

#include "slist.h"
#include "pages.h"
#include "inode.h"

#define NAME_LEN 16
#define ENT_SIZE (sizeof(dir_ent))
#define ENT_PER_PAGE (PAGE_SIZE / ENT_SIZE)

typedef struct dir_ent {

	/**
	 * THe inode number.
	 */
	int inode_num;

	/**
	 * The file name.
	 */
	char filename[NAME_LEN];

} dir_ent;

typedef struct dir_page {

	/**
	 * An array of filenames. 0 is null.
	 */
	dir_ent files[ENT_PER_PAGE];

} dir_page;

/**
 * Get the name of the iith file in the directory.
 *
 * Return 0 if invalid;
 */
dir_ent* directory_get(int dirnode, int ii);
void directory_init(int dirnode, int new);

/**
 * Given a directory and a filename, find the file's entry index.
 *
 * Return -ENOENT on failure.
 */
int directory_lookup(int dirnode, const char* name);

/**
 * Given a path to a file or directory, return the corresponding inode.
 */
int tree_lookup(const char* path);

/**
 * Add a file to the dir.
 */
int directory_put(int dirnode, const char* name, int inum);

/**
 * Delete the directory. It must be empty.
 */
int directory_delete(int dirnode, const char* name);

/**
 * List the names of all files in the directory.
 */
slist* directory_list(int dirnode);
void print_directory(int dirnode);

#endif

