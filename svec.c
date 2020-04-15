/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */
// Modified by Jason Sauntry: adapted to ints.

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdio.h>

#include "svec.h"

svec*
make_svec()
{
	svec* sv = calloc(1, sizeof(svec));

	sv->size = 0;
	sv->capacity = 2;
	sv->data = calloc(sv->capacity, sizeof(char*));
	return sv;
}

void
free_svec(svec* sv)
{
	// Free data.
	free(sv->data);
	
	// Free svec
	free(sv);
}

// Returns a pointer to our copy of the string.
int svec_get(svec* sv, int ii)
{
	assert(ii >= 0 && ii < sv->size);
	return sv->data[ii];
}

// We copy the string, own our own copy.
void
svec_put(svec* sv, int ii, int item)
{
	assert(ii >= 0 && ii < sv->size);
	sv->data[ii] = item;
}

/**
 * Double the capacity, leaving all data intact.
 */
void svec_grow(svec* sv)
{
	sv->capacity *= 2;
	sv->data = realloc(sv->data, sv->capacity * sizeof(char*));
}

void
svec_push_back(svec* sv, int item)
{
	int ii = sv->size;

	if (ii >= sv->capacity) {
		svec_grow(sv);
	}

	sv->size += 1;
	svec_put(sv, ii, item);
}

void
svec_swap(svec* sv, int i, int j)
{
	// No need to use put and get, we'll just access the underlying array
	// directly. Saves us allocating new memory.
	assert(0 <= i && i < sv->size);
	assert(0 <= j && j < sv->size);

	int tmp = sv->data[i];
	sv->data[i] = sv->data[j];
	sv->data[j] = tmp;
}

int svec_contains(svec* sv, int item) {
	for (int i = 0; i < sv->size; i++) {
		if (svec_get(sv, i) == item) {
			return 1;
		}
	}
	return 0;
}
