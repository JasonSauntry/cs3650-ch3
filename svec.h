/* This file is lecture notes from CS 3650, Fall 2018 */
/* Author: Nat Tuck */
// Modified by Jason Sauntry: adapted to ints.

#ifndef SVEC_H
#define SVEC_H

/**
 * A string vector.
 */
typedef struct svec {

	/**
	 * The number of elements currently in the svec.
	 */
	int size;

	/**
	 * The maximum number of elements we can hold without reallocating memory.
	 */
	int capacity;

	/**
	 * The array containing the data. All data is owned by the svec, will be
	 * cleaned up on call to free_svec.
	 */
	int* data;

} svec;

/**
 * Make a new empty svec.
 */
svec* make_svec();

/**
 * Free {@code sv} and all data contained within it.
 */
void  free_svec(svec* sv);

/**
 * Get the {@code ii}th element of {@code sv}.
 */
int svec_get(svec* sv, int ii);

/**
 * Set the {@code ii}th element of {@code sv} to {@code item}.
 */
void  svec_put(svec* sv, int ii, int item);

/**
 * Append to the end of the svec.
 *
 * @param sv	The svec to append to.
 * @param item	The string to append.
 */
void svec_push_back(svec* sv, int item);
void svec_swap(svec* sv, int ii, int jj);

int svec_contains(svec* sv, int item);

#endif
