#ifndef ACCRUAL_DETECTOR_ALGORITHM_MEMORY_LEAK_DETECTOR_H
#define ACCRUAL_DETECTOR_ALGORITHM_MEMORY_LEAK_DETECTOR_H

#include <stdio.h>
#include <stdlib.h>

// Define DEBUG_MEMORY for detailed logging
//#define DEBUG_MEMORY

typedef struct MemoryBlock {
    void *address;              // Address of the allocated memory
    size_t size;                // Size of the allocated memory
    const char *file;           // File in which allocation was made
    int line;                   // Line number of the allocation
    struct MemoryBlock *next;   // Next memory block in the list
} MemoryBlock;

extern MemoryBlock *head;       // Declare head

// Redefine malloc, calloc and free to test_malloc, test_calloc and test_free respectively
#define malloc(size) test_malloc(size, __FILE__, __LINE__, #size)
#define calloc(num, size) test_calloc(num, size, __FILE__, __LINE__, #num " * " #size)
#define free(ptr) test_free(ptr, __FILE__, __LINE__)

void *test_malloc(size_t size, const char *file, int line, const char *ptr_name);

void *test_calloc(size_t num, size_t size, const char *file, int line, const char *ptr_name);

void test_free(void *ptr, const char *file, int line);

void check_for_leaks();

#endif //ACCRUAL_DETECTOR_ALGORITHM_MEMORY_LEAK_DETECTOR_H