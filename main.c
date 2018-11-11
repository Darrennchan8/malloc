#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "malloc.h"

#define sbrk_should(option) assert_sbrk_should(option, -1)
#define INITIALIZE 0
#define DECREASE 1
#define STAY_THE_SAME 2
#define INCREASE 3

void* previous_sbrk;

void assert_that(char* message, int expression) {
    if (expression) {
        printf("PASSED: %s\n", message);
    } else {
        printf("FAILED: %s\n", message);
        abort();
    }
}

void assert_sbrk_should(int option, int by) {
    void* current_sbrk = sbrk(0);
    char message[150];
    switch (option) {
        case INITIALIZE:
            printf("Initial program break: %p\n", current_sbrk);
            previous_sbrk = current_sbrk;
            return;
        case DECREASE:
            if (by >= 0) {
                sprintf(message, "Program break should decrease by %d (previous=%p, current=%p).", by, previous_sbrk, current_sbrk);
                assert_that(message, previous_sbrk - by == current_sbrk);
            } else {
                sprintf(message, "Program break should decrease (previous=%p, current=%p).", previous_sbrk, current_sbrk);
                assert_that(message, current_sbrk < previous_sbrk);
            }
            break;
        case STAY_THE_SAME:
            sprintf(message, "Program break should stay the same (previous=%p, current=%p).", previous_sbrk, current_sbrk);
            assert_that(message, current_sbrk == previous_sbrk);
            break;
        case INCREASE:
            if (by >= 0) {
                sprintf(message, "Program break should increase by %d (previous=%p, current=%p).", by, previous_sbrk, current_sbrk);
                assert_that(message, previous_sbrk + by == current_sbrk);
            } else {
                sprintf(message, "Program break should increase (previous=%p, current=%p).", previous_sbrk, current_sbrk);
                assert_that(message, current_sbrk > previous_sbrk);
            }
            break;
        default:
            assert_that("Invalid option for sbrk_should.", 0);
    }
    previous_sbrk = current_sbrk;
}

void assert_eq(int p1, int p2) {
    char message[50];
    sprintf(message, "%d == %d", p1, p2);
    assert_that(message, p1 == p2);
}

void assert_ptr_eq(void* p1, void* p2) {
    char message[50];
    sprintf(message, "%p == %p", p1, p2);
    assert_that(message, p1 == p2);
}

void assert_ptr_neq(void* p1, void* p2) {
    char message[50];
    sprintf(message, "%p != %p", p1, p2);
    assert_that(message, p1 != p2);
}

void record_memory_leak_for_block(struct allocation_block* block, size_t* internal, size_t* external) {
    *external += block->free ? block->size : 0;
    *internal += block->free ? 0 : block->size - block->requested_size;
}

void get_total_memory_leak(size_t* internal, size_t* external) {
    *internal = 0;
    *external = 0;
    for (struct allocation_block* block = allocation_head; block; block = block->next) {
        record_memory_leak_for_block(block, internal, external);
    }
}

void assert_total_memleak_eq(long exp_internal, long exp_external) {
    size_t actual_internal, actual_external;
    get_total_memory_leak(&actual_internal, &actual_external);
    char message[100];
    if (exp_internal != -1) {
        sprintf(message, "Expect total internal memory leak to be %lu bytes, got %lu.", exp_internal, actual_internal);
        assert_that(message, exp_internal == actual_internal);
    }
    if (exp_external != -1) {
        sprintf(message, "Expect total external memory leak to be %lu bytes, got %lu.", exp_external, actual_external);
        assert_that(message, exp_external == actual_external);
    }
}

void assert_memleak_eq(struct allocation_block* block, long exp_internal, long exp_external) {
    size_t actual_internal = 0, actual_external = 0;
    record_memory_leak_for_block(block, &actual_internal, &actual_external);
    char message[100];
    if (exp_internal != -1) {
        sprintf(message, "Expect internal memory leak for %p to be %lu bytes, got %lu.", block, exp_internal, actual_internal);
        assert_that(message, exp_internal == actual_internal);
    }
    if (exp_external != -1) {
        sprintf(message, "Expect external memory leak for %p to be %lu bytes, got %lu.", block, exp_external, actual_external);
        assert_that(message, exp_external == actual_external);
    }
}

void assert_memleak_for_allocation_eq(void* allocation, long exp_internal, long exp_external) {
    assert_memleak_eq(find_allocation_block_for_allocation(allocation), exp_internal, exp_external);
}

void print_total_memory_leak() {
    size_t internal, external;
    get_total_memory_leak(&internal, &external);
    printf("Total internal memory leak: %lu bytes\nTotal external memory leak: %lu bytes.\n", internal, external);
}

int main() {
    sbrk_should(INITIALIZE);

    // Tests that alignment is 8 bytes.
    char* oneChar = calloc(1, sizeof(char));
    sbrk_should(INCREASE);

    char* twoChars = realloc(oneChar, 8 * sizeof(char));
    sbrk_should(STAY_THE_SAME);
    assert_ptr_eq(oneChar, twoChars);

    char* nineChars = calloc(9, sizeof(char));
    sbrk_should(INCREASE);
    assert_ptr_neq(twoChars, nineChars);

    free(nineChars);
    char* name = realloc(twoChars, 12 * sizeof(char));
    sprintf(name, "Darren Chan");
    assert_that("%s == Darren Chan", strcmp(name, "Darren Chan\0") == 0);
    assert_ptr_eq(oneChar, name);
    sbrk_should(STAY_THE_SAME);

    char* twoCharsAgain = malloc(2 * sizeof(char));
    assert_ptr_neq(twoChars, twoCharsAgain);
    sbrk_should(STAY_THE_SAME);
    free(twoCharsAgain);

    // Tests that we don't have any fragmentation.
    // Allocate 16 bytes, free
    // Allocate 8 bytes, free
    // Allocate again should use the same pointer as the last 2 allocations.
    char* eightChars = malloc(8 * sizeof(char));
    assert_ptr_eq(twoCharsAgain, eightChars);
    sbrk_should(STAY_THE_SAME);

    // Tests that calloc clears the bits.
    int* numbersToTwenty = calloc(20, sizeof(int));
    sbrk_should(INCREASE);
    for (int i = 1; i < 20; i++) {
        numbersToTwenty[i] = i;
    }
    assert_that("Calloc should clear the bits.", numbersToTwenty[0] == 0);

    // Tests that realloc correctly splits the block.
    char* tenChars = realloc(numbersToTwenty, 10 * sizeof(char));
    assert_ptr_eq(numbersToTwenty, tenChars);
    sbrk_should(STAY_THE_SAME);
    int* numbersToSix = calloc(6, sizeof(int));
    sbrk_should(STAY_THE_SAME);
    assert_ptr_neq(numbersToSix, numbersToTwenty);
    for (int i = 1; i < 6; i++) {
        // Ensures that calloc correctly splits the block contiguously and aligns.
        *((int*)(tenChars + 16 + sizeof(struct allocation_block) / sizeof(char)) + i) = i;
    }
    for (int i = 0; i < 6; i++) {
        assert_eq(i, numbersToSix[i]);
    }
    free(numbersToSix);
    free(tenChars);
    int* numbersToTwentyAgain = malloc(20 * sizeof(int));
    assert_ptr_eq(tenChars, numbersToTwentyAgain);
    sbrk_should(STAY_THE_SAME);

    // Tests that malloc internally uses the best-fit algorithm.
    free(name);
    free(eightChars);
    free(numbersToTwentyAgain);
    char* a = malloc(16);
    char* b = malloc(8);
    char* c = malloc(8);
    char* d = malloc(8);
    free(a);
    free(c);
    char* e = malloc(8);
    assert_ptr_eq(c, e);
    assert_ptr_neq(a, e);
    free(b);
    free(d);
    free(e);
    sbrk_should(STAY_THE_SAME);

    // Tests that allocating uses the tail if free, even if we need to increment sbrk.
    long* bigArray = calloc(25, sizeof(long));
    sbrk_should(INCREASE);
    long* bigArray2 = realloc(bigArray, 26 * sizeof(long));
    assert_sbrk_should(INCREASE, sizeof(long));
    assert_ptr_eq(bigArray, bigArray2);
    free(bigArray2);
    long* bigArray3 = calloc(27, sizeof(long));
    assert_sbrk_should(INCREASE, sizeof(long));
    assert_ptr_eq(bigArray, bigArray2);
    assert_total_memleak_eq(0, 0);

    // Test for expected individual memory leaks.
    char* block1 = realloc(bigArray3, sizeof(char));
    assert_ptr_eq(bigArray3, block1);
    assert_memleak_for_allocation_eq(block1, 7, 0);
    char* block2 = realloc(block1, sizeof(char) * 2);
    assert_ptr_eq(block1, block2);
    assert_memleak_for_allocation_eq(block2, 6, 0);
    free(block2);
    char* block3 = calloc(8, sizeof(char));
    assert_memleak_for_allocation_eq(block3, 0, 0);
    char* block4 = calloc(7, sizeof(char));
    assert_ptr_neq(block3, block4);
    assert_memleak_for_allocation_eq(block4, 1, 0);
    free(block3);
    assert_memleak_for_allocation_eq(block3, 0, 8);
    char* block5 = malloc(16 * sizeof(char));
    assert_ptr_neq(block4, block5);
    assert_memleak_for_allocation_eq(block5, 0, 0);
    free(block4);
    assert_memleak_eq(allocation_head, 0, 16 + sizeof(struct allocation_block));
    free(block5);
    char* block6 = malloc(5 * sizeof(char));
    assert_ptr_eq(allocation_head + 1, block6);
    assert_memleak_eq(allocation_head, 3, 0);
    // Reallocate rest of space perfectly to start out with clean slate when calculating total memory leak.
    long* allocateAllOfSpace = realloc(block6, 27 * sizeof(long));
    sbrk_should(STAY_THE_SAME);

    // Test for expected total memory leaks.
    char* cArr = calloc(9, sizeof(char));
    sbrk_should(INCREASE);
    assert_total_memleak_eq(7, 0);
    char* cArr2 = realloc(cArr, sizeof(char));
    sbrk_should(STAY_THE_SAME);
    assert_ptr_eq(cArr, cArr2);
    assert_total_memleak_eq(15, 0);
    char* cArr3 = calloc(4, sizeof(char));
    sbrk_should(INCREASE);
    assert_ptr_neq(cArr2, cArr3);
    assert_total_memleak_eq(15 + 4, 0);
    char* cArr4 = realloc(cArr2, 24 * sizeof(char));
    sbrk_should(INCREASE);
    assert_ptr_neq(cArr2, cArr4);
    assert_total_memleak_eq(4, 16);
    char* cArr5 = realloc(cArr3, 5 * sizeof(char));
    sbrk_should(STAY_THE_SAME);
    assert_ptr_eq(cArr3, cArr5);
    assert_total_memleak_eq(3, 16);
    print_total_memory_leak();
}
