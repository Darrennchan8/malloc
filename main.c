#include <stdio.h>
#include <unistd.h>
#include "malloc.h"

int main() {
    printf("Initial sbrk: %p\n", sbrk(0));

    char* oneChar = calloc(1, sizeof(char));
    printf("sbrk should increase: %p\n", sbrk(0));

    char* twoChars = realloc(oneChar, 2 * sizeof(char));
    printf("sbrk shouldn't change: %p\n", sbrk(0));
    printf("%p == %p\n", (void*) oneChar, (void*) twoChars);

    char* nineChars = realloc(twoChars, 9 * sizeof(char));
    printf("sbrk should increase: %p\n", sbrk(0));
    printf("%p != %p\n\n", (void*) twoChars, (void*) nineChars);

    free(nineChars);
    char* name = calloc(12, sizeof(char));
    name[0] = 'D';
    name[1] = 'a';
    name[2] = 'r';
    name[3] = 'r';
    name[4] = 'e';
    name[5] = 'n';
    name[6] = ' ';
    name[7] = 'C';
    name[8] = 'h';
    name[9] = 'a';
    name[10] = 'n';
    name[11] = '\0';
    printf("%s == Darren Chan\n", name);
    printf("%p == %p\n", (void*) oneChar, (void*) name);
    printf("sbrk shouldn't change: %p\n\n", sbrk(0));

    char* twoCharsAgain = malloc(2 * sizeof(char));
    printf("%p != %p\n", (void*) twoChars, (void*) twoCharsAgain);
    printf("sbrk shouldn't change: %p\n", sbrk(0));
    free(twoCharsAgain);

    char* eightChars = malloc(8 * sizeof(char));
    printf("%p == %p\n", (void*) twoCharsAgain, (void*) eightChars);
    printf("sbrk shouldn't change: %p\n\n", sbrk(0));

    int* numbersToEighteen = calloc(18, sizeof(int));
    printf("sbrk should increase: %p\n", sbrk(0));
    for (int i = 1; i < 18; i++) {
        numbersToEighteen[i] = i;
    }
    for (int i = 0; i < 18; i++) {
        printf("%d ", numbersToEighteen[i]);
    }
    printf("\n");
    char* tenChars = realloc(numbersToEighteen, 10 * sizeof(char));
    printf("%p == %p\n", (void*) numbersToEighteen, (void*) tenChars);
    printf("sbrk shouldn't change: %p\n", sbrk(0));
    int* numbersToSix = calloc(6, sizeof(int));
    printf("sbrk shouldn't change: %p\n", sbrk(0));
    printf("%p != %p\n", (void*) numbersToSix, (void*) numbersToEighteen);
    for (int i = 1; i < 6; i++) {
        numbersToEighteen[i + 12] = i;
    }
    for (int i = 0; i < 6; i++) {
        printf("%d ", numbersToSix[i]);
    }
    printf("\n");
    free(numbersToSix);
    free(tenChars);
    int* numbersToEighteenAgain = malloc(18 * sizeof(int));
    printf("%p == %p\n", (void*) tenChars, (void*) numbersToEighteenAgain);
    printf("sbrk shouldn't change: %p\n", sbrk(0));
    for (int i = 1; i < 18; i++) {
        numbersToEighteen[i] = i;
    }
    for (int i = 0; i < 18; i++) {
        printf("%d ", numbersToEighteen[i]);
    }
}
