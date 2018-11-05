//
// Created by Darren Chan on 11/3/18.
//
#include <sys/types.h>

#ifndef ASSIGN3_ASSIGN3_H
#define ASSIGN3_ASSIGN3_H

void* malloc(size_t size);

void* realloc(void* ptr, size_t size);

void* calloc(size_t num_elements, size_t element_size);

void free(void* ptr);

#endif //ASSIGN3_ASSIGN3_H
