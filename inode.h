#ifndef INODE_H
#define INODE_H

#include <sys/time.h>

#include "pages.h"

#define PAGE_ARRAY_SIZE 254

#define DIRMODE  040000
#define SLNMODE 0120000



typedef struct inode {
    int mode; // permission & type; zero for unused
    int size; // bytes, regardless of type.
    short int directory; // Is it a dir?
    short int link; // Is it a symlink?
    int refs; // Ref counter for hard links.
    struct timespec last_access;
    struct timespec last_modified;
    
    /**
     * The pages containing the data. No indirect links as of now. 0 is null.
     * Null-terminated, thus last entry cannot ever be used.
     */
    int pages[PAGE_ARRAY_SIZE];
} inode;

void print_inode(inode* node);
inode* get_inode(int inum);
int alloc_inode();
void free_inode();
int inode_get_pnum(inode* node, int fpn);

/**
 * Allocate pages for the appropriate number of inodes.
 * 
 * May only be done at filesystem initialization. Will clobber data. Must be
 * done after bitmap initialization, becuase that will clobber this.
 */
void alloc_inode_pages();

#endif
