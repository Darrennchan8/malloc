#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "malloc.h"

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

void sbrk_should(int option) {
    void* current_sbrk = sbrk(0);
    char message[100];
    switch (option) {
        case DECREASE:
            sprintf(message, "Program break should decrease (previous=%p, current=%p).", previous_sbrk, current_sbrk);
            assert_that(message, current_sbrk < previous_sbrk);
            break;
        case STAY_THE_SAME:
            sprintf(message, "Program break should stay the same (previous=%p, current=%p).", previous_sbrk, current_sbrk);
            assert_that(message, current_sbrk == previous_sbrk);
            break;
        case INCREASE:
            sprintf(message, "Program break should increase (previous=%p, current=%p).", previous_sbrk, current_sbrk);
            assert_that(message, current_sbrk > previous_sbrk);
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

int main() {
    previous_sbrk = sbrk(0);
    printf("Initial sbrk: %p\n", previous_sbrk);

    // Tests that alignment is 8 bytes.
    char* oneChar = calloc(1, sizeof(char));
    sbrk_should(INCREASE);

    char* twoChars = realloc(oneChar, 8 * sizeof(char));
    sbrk_should(STAY_THE_SAME);
    assert_ptr_eq(oneChar, twoChars);

    char* nineChars = realloc(twoChars, 9 * sizeof(char));
    sbrk_should(INCREASE);
    assert_ptr_neq(twoChars, nineChars);

    free(nineChars);
    char* name = calloc(12, sizeof(char));
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
}
