#include "memory_leak_detector.h"

MemoryBlock *head = NULL;       // Define head


void *test_malloc(size_t size, const char *file, int line, const char *ptr_name) {
#undef malloc
    void *ptr = malloc(size);
    if (ptr != NULL) {
#ifdef DEBUG_MEMORY
        printf("malloc: %zu bytes for '%s' at %p (file: %s, line: %d)\n", size, ptr_name, ptr, file, line);
#endif
        MemoryBlock *block = malloc(sizeof(MemoryBlock));
        if (block) {
            block->address = ptr;
            block->size = size;
            block->file = file;
            block->line = line;
            block->next = head;
            head = block;
        }
    }
#define malloc(size) test_malloc_named(size, __FILE__, __LINE__, #size)
    return ptr;
}

void *test_calloc(size_t num, size_t size, const char *file, int line, const char *ptr_name) {
#undef calloc
    void *ptr = calloc(num, size);
    if (ptr != NULL) {
#ifdef DEBUG_MEMORY
        printf("calloc: %zu bytes for '%s' at %p (file: %s, line: %d)\n", size, ptr_name, ptr, file, line);
#endif
#undef malloc
        MemoryBlock *block = malloc(sizeof(MemoryBlock));
#define malloc(size) test_malloc_named(size, __FILE__, __LINE__, #size)
        if (block) {
            block->address = ptr;
            block->size = size;
            block->file = file;
            block->line = line;
            block->next = head;
            head = block;
        }
    }
#define calloc(num, size) test_calloc(num, size, __FILE__, __LINE__, #num " * " #size)
    return ptr;
}

void test_free(void *ptr, const char *file, int line) {
#undef free
    if (ptr == NULL) {
#ifdef DEBUG_MEMORY
        printf("Attempt to free a NULL pointer (file: %s, line: %d)\n", file, line);
#endif
        return;
    }

    MemoryBlock **current = &head;
    while (*current != NULL) {
#ifdef DEBUG_MEMORY
        printf("free: %p (file: %s, line: %d)\n", ptr, file, line);
#endif
        if ((*current)->address == ptr) {
            MemoryBlock *temp = *current;
            *current = (*current)->next;
            free(temp->address);
            free(temp);
            return;
        }
        current = &(*current)->next;
    }
#ifdef DEBUG_MEMORY
    printf("Attempt to free an untracked pointer: %p (file: %s, line: %d)\n", ptr, file, line);
#endif
#define free(ptr) test_free(ptr, __FILE__, __LINE__)
}


void check_for_leaks() {
    MemoryBlock *current = head;
    if (current == NULL) {
        printf("\n\nNo memory leaks detected.\n");
    } else {
        printf("\n\nMemory leaks detected:\n");
        while (current != NULL) {
            printf("Leak of size %zu at %p (allocated in %s:%d)\n",
                   current->size, current->address, current->file, current->line);
            MemoryBlock *temp = current;
            current = current->next;
#undef free
            free(temp);
#define free(ptr) test_free(ptr, __FILE__, __LINE__)
        }
    }
}