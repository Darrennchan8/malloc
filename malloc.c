//
// Created by Darren Chan on 11/3/18.
//

#include <unistd.h>
#include <string.h>
#include "malloc.h"
#define META_SIZE sizeof(struct allocation_block)
#define TRUE 1
#define FALSE 0

// TODO: allocation_blocks go sequentially upwards. We can optimize our program with this.
struct allocation_block {
    size_t size;
    struct allocation_block *next;
    struct allocation_block *previous;
    int free;
};

struct allocation_block* allocation_head = NULL;
struct allocation_block* allocation_tail = NULL;

size_t align(size_t size) {
    return size + (8 - ((int) size + META_SIZE) % 8) % 8;
}

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
 * Links a new block after allocation_tail with size `size` or extends allocation_tail if free.
 *
 * @param size The size needed for the allocation block.
 * @return The allocation block, or NULL if sbrk failed.
 */
struct allocation_block* request_space(size_t size) {
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

void split_if_possible(struct allocation_block* left, size_t size) {
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
    }
}

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
        memcpy(previous_block + 1, block + 1, block->size);
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

void* malloc(size_t size) {
    if (size <= 0) {
        return NULL;
    }
    size = align(size);
    struct allocation_block* allocated_block = find_free_block_best_fit(size);
    if (allocated_block) {
        allocated_block->free = FALSE;
        split_if_possible(allocated_block, size);
    } else {
        // Allocate a new block.
        allocated_block = request_space(size);
    }
    return allocated_block + 1;
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

void* realloc(void* ptr, size_t size) {
    if (size <= 0) {
        free(ptr);
        return malloc(size);
    }
    size = align(size);
    struct allocation_block* target_block = allocation_head;
    for (; target_block && target_block + 1 != ptr; target_block = target_block->next);
    if (target_block) {
        if (target_block->size >= size) {
            split_if_possible(target_block, size);
            return target_block + 1;
        } else {
            // TODO: Handle when the previous block is free and the next block might be free.
            void* new_ptr = malloc(size);
            size_t copy_size = target_block->size > size ? size : target_block->size;
            memcpy(new_ptr, target_block + 1, copy_size);
            merge_adjacent_free(target_block);
            return new_ptr;
        }
    } else {
        // Either ptr is NULL or ptr doesn't have an associated target_block.
        return malloc(size);
    }
}

void* calloc(size_t num_elements, size_t element_size) {
    size_t size = align(num_elements * element_size);
    void* ptr = malloc(size);
    memset(ptr, 0, size);
    return ptr;
}
