//
// Created by Darren Chan on 11/3/18.
//

#include <unistd.h>
#include <string.h>
#include "malloc.h"

#define META_SIZE sizeof(struct allocation_block)
#define align(size) size + (8 - ((int) size + META_SIZE) % 8) % 8
#define TRUE 1
#define FALSE 0

struct allocation_block* allocation_head = NULL;
struct allocation_block* allocation_tail = NULL;

/**
 * Finds the best-fitting (smallest possible) free block of size at least `size`, or NULL if it doesn't exist.
 *
 * @param size The size needed for the allocation block.
 * @return The best-fitting allocation block, or NULL if there aren't any of enough size.
 */
struct allocation_block* find_free_block_best_fit(size_t size) {
    struct allocation_block* best_fit = NULL;
    for (struct allocation_block *last = allocation_head; last; last = last->next) {
        if (last->free && last->size >= size) {
            best_fit = best_fit && best_fit->size <= last->size ? best_fit : last;
        }
    }
    return best_fit;
}

/**
 * Finds the allocation_block associated with a particular memory allocation, or NULL if none match.
 *
 * @param ptr The memory allocation pointer to the data (e.g: returned by a function like `malloc`).
 * @return The allocation_block associated with ptr, or NULL if none match.
 */
struct allocation_block* find_allocation_block_for_allocation(void* ptr) {
    for (struct allocation_block* block = allocation_head; block; block = block->next) {
        if (block + 1 == ptr) {
            return block;
        }
        // Since the heap grows upwards, we can stop checking if we start seeing allocation blocks that are greater than
        // our target ptr.
        if ((void*) block > ptr) {
            break;
        }
    }
    return NULL;
}

/**
 * Links a new block after allocation_tail with size `size` or extends allocation_tail if free.
 *
 * @param size The size needed for the allocation block.
 * @return The allocation block, or NULL if sbrk failed.
 */
struct allocation_block* request_space(size_t size) {
    // Extend and reuse the tail if possible.
    if (allocation_tail && allocation_tail->free) {
        if (sbrk((int) (size - allocation_tail->size)) == (void*) -1) {
            return NULL;
        }
        allocation_tail->free = FALSE;
        allocation_tail->size = size;
        return allocation_tail;
    }
    struct allocation_block *block = sbrk(META_SIZE + size);
    if (block == (void*) -1) {
        return NULL;
    }

    // Initialize the new tail.
    block->free = FALSE;
    block->next = NULL;
    block->size = size;
    if (allocation_tail) {
        allocation_tail->next = block;
    } else {
        allocation_head = block;
    }
    block->previous = allocation_tail;
    return allocation_tail = block;
}

/**
 * Partitions an allocation_block and splits into left and right allocation_blocks, if the new right portion can hold at
 * least 8 bytes in addition to the size of the meta information.
 *
 * @param left The allocation_block to split.
 * @param size The required data size for the left block.
 * @return The right allocation_block, or NULL if we aren't able to split.
 */
struct allocation_block* split_if_possible(struct allocation_block* left, size_t size) {
    size_t right_size = left->size - size;
    if (right_size >= 8 + META_SIZE) {
        left->size = size;
        struct allocation_block* right = (void*) (left + 1) + left->size;
        right->size = right_size - META_SIZE;
        right->free = TRUE;
        right->previous = left;
        right->next = left->next;
        left->next = right;
        if (right->next) {
            right->next->previous = right;
        }
        if (left == allocation_tail) {
            allocation_tail = right;
        }
        return right;
    }
    return NULL;
}

/**
 * Merges the current block with surrounding blocks if available and returns a pointer to the merged block. If the
 * current block is not free, the merged block is guaranteed to have the same data as the original block after merging.
 *
 * @param block The block to merge with adjacent blocks.
 * @return A pointer to the merged block.
 */
struct allocation_block* merge_adjacent_free(struct allocation_block* block) {
    struct allocation_block* previous_block = block->previous;
    struct allocation_block* next_block = block->next;
    if (previous_block && previous_block->free) {
        if (block == allocation_tail) {
            allocation_tail = previous_block;
        }
        previous_block->next = block->next;
        if (next_block) {
            next_block->previous = previous_block;
        }
        previous_block->size += META_SIZE + block->size;
        if (!block->free) {
            memcpy(previous_block + 1, block + 1, block->size);
        }
        block = previous_block;
    }
    if (next_block && next_block->free) {
        if (next_block == allocation_tail) {
            allocation_tail = block;
        }
        block->next = next_block->next;
        if (next_block->next) {
            next_block->next->previous = block;
        }
        block->size += META_SIZE + next_block->size;
    }
    return block;
}

/**
 * merge_free_right is the same as `merge_adjacent_free` but it only merges with the right block when avaliable. Since
 * the pointer to the merged block will be the same with and without merging, nothing is returned.
 *
 * @param block The block to merge with the right block.
 */
void merge_free_right(struct allocation_block* block) {
    // Pretend like the left block isn't free when necessary.
    if (block->previous && block->previous->free) {
        block->previous->free = FALSE;
        merge_adjacent_free(block);
        block->previous->free = TRUE;
    } else {
        merge_adjacent_free(block);
    }
}

void* malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }
    size_t aligned_size = align(size);
    struct allocation_block* allocated_block = find_free_block_best_fit(aligned_size);
    if (allocated_block) {
        allocated_block->free = FALSE;
        split_if_possible(allocated_block, aligned_size);
    } else {
        // Allocate a new block.
        allocated_block = request_space(aligned_size);
#ifdef __DEBUG__
        allocated_block->requested_size = size;
#endif
    }
    return allocated_block + 1;
}

void* calloc(size_t num_elements, size_t element_size) {
    size_t size = align(num_elements * element_size);
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}

void* realloc(void* ptr, size_t size) {
    size = align(size);
    struct allocation_block* target_block = find_allocation_block_for_allocation(ptr);
    if (size <= 0 || !target_block) {
        free(ptr);
        return malloc(size);
    }
    size_t leftAvailable = target_block->previous && target_block->previous->free ? target_block->previous->size : 0;
    size_t rightAvailable = target_block->next && target_block->next->free ? target_block->next->size : 0;

    // When reallocating a block, here are the priorities that we will partition by.
    // 1. Reuse (current block + extend right).
    //    Since merging with right is an O(1) operation, we can preemptively merge with the right block and then
    //    repartition over just partitioning the current block to avoid 2 consecutive free blocks to the right,
    //    limiting fragmentation. If there isn't a next block (i.e: the current block is the tail), we can simply extend
    //    sbrk, to be as efficient as possible.
    // 2. Reuse all adjacent blocks.
    //    Since merging with left is an O(size) operation, we should merge with left only when we can't do (1).
    // 3. Push a new tail onto the allocation blocks.
    //    Since this has the same time complexity as (2) but (2) limits fragmentation, we should only do this when all
    //    other options can't be chosen.
    if (rightAvailable + target_block->size >= size) {
        merge_free_right(target_block);
        split_if_possible(target_block, size);
        return target_block + 1;
    } else if (target_block == allocation_tail) {
        target_block->free = TRUE;
        request_space(size);
        return target_block + 1;
    } else if (leftAvailable + rightAvailable + target_block->size >= size) {
        target_block = merge_adjacent_free(target_block);
        split_if_possible(target_block, size);
        return target_block + 1;
    } else {
        // size is guaranteed to be less than target_block's size.
        void* new_ptr = malloc(size);
        memcpy(new_ptr, target_block + 1, size);
        target_block->free = TRUE;
        merge_adjacent_free(target_block);
        return new_ptr;
    }
}

void free(void* ptr) {
    for (struct allocation_block* block = allocation_head; block; block = block->next) {
        if (block + 1 == ptr) {
            block->free = TRUE;
            merge_adjacent_free(block);
            return;
        }
    }
}
