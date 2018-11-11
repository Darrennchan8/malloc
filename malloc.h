/*
 * malloc.h
 *
 * Malloc library: malloc/calloc/realloc/free implementation.
 * Does not include these standard (ANSI/SVID/...) functions:
 *   memalign(size_t alignment, size_t n);
 *   valloc(size_t n);
 *   mallinfo();
 *   mallopt(int parameter_number, int parameter_value);
 *
 * Written by Darren Chan <darrennchan8@gmail.com>
 */
#include <sys/types.h>

// Typical uses of *alloc and free don't require knowing a lot of the calculations that we need to know, so only provide
// debug information when necessary (e.g: for testing).
#define __DEBUG__

#ifndef ASSIGN3_ASSIGN3_H
#define ASSIGN3_ASSIGN3_H

#ifdef __DEBUG__

struct allocation_block* allocation_head;
struct allocation_block* allocation_tail;

/** Documentation is available in malloc.c */
struct allocation_block* find_allocation_block_for_allocation(void* ptr);

#endif

struct allocation_block {
#ifdef __DEBUG__
    size_t requested_size;
#endif
    size_t size;
    struct allocation_block *next;
    struct allocation_block *previous;
    int free;
};

/**
 * Allocates some memory of size `size` and returns a pointer to the start of the block. The size allocated is
 * guaranteed to be aligned to 8 bytes.
 *
 * @param size The size of the block to allocate.
 * @return A pointer to the start of the block of memory.
 */
void* malloc(size_t size);

/**
 * Allocates some memory of size `num_elements * element_size` and returns a pointer to the start of the block. The size
 * allocated is guaranteed to be aligned to 8 bytes. Clears the memory to be all zeroes.
 *
 * @param num_elements The number of units to allocate.
 * @param element_size The size of each unit.
 * @return A pointer to the start of the block of memory.
 */
void* calloc(size_t num_elements, size_t element_size);

/**
 * Resizes a previous allocation of memory to be of size `size`. Frees the previous allocation as necessary.
 *
 * @param ptr A pointer referencing the previous allocation, should be returned by `*alloc`.
 * @param size The size to change to. Aligned to 8 bytes.
 * @return A pointer to the start of the new/original block of memory.
 */
void* realloc(void* ptr, size_t size);

/**
 * Frees a allocated block of memory previously allocated by `*alloc`.
 *
 * @param ptr The pointer returned by `*alloc`.
 */
void free(void* ptr);

#endif //ASSIGN3_ASSIGN3_H
