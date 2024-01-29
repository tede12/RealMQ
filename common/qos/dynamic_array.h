//  =====================================================================
//  dynamic_array.h
//
//  Dynamic array implementation
//  =====================================================================

#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// Message structure
typedef struct {
    uint64_t id;
    char *content;
} Message;


// Atomic for thread-safe unique message ID generation
extern volatile uint64_t atomic_msg_id;

// Mutex for thread-safety while accessing the IDs array
extern pthread_mutex_t msg_ids_mutex;

// Dynamic array for storing message IDs awaiting ACK
typedef struct {
    void **data;            // Void pointer to the data for genericity
    size_t size;            // Number of elements in the array
    size_t capacity;        // Total capacity of the array
    size_t element_size;    // Size of each element in the array
} DynamicArray;

// Global dynamic array for storing message IDs awaiting ACK
extern DynamicArray g_array;

// Initialize dynamic array with a certain capacity
void init_dynamic_array(DynamicArray *array, size_t initial_capacity, size_t element_size);

// Resize the dynamic array
void resize_dynamic_array(DynamicArray *array);

// Add an item to the dynamic array
void add_to_dynamic_array(DynamicArray *array, void *element);

// Create a new message
void *create_element(const char *content);

// Copy an element
void copy_element(void *src, void *dst, size_t element_size);

// Debugging function to print the dynamic array
void print_dynamic_array(DynamicArray *array);

// Generate a unique message ID
uint64_t generate_unique_message_id();

// Reset the message ID
void reset_message_id();

// Set the message ID
void set_message_id(uint64_t value);

// Get an element by index
void *get_element_by_index(DynamicArray *array, long long index);

// Clean all elements from the array
size_t clean_all_elements(DynamicArray *array);

// Remove a message from the array
long long remove_element_by_id(DynamicArray *array, uint64_t msg_id, bool use_interpolation_search, bool remove_element);

// Free a message
void release_element(void *element, size_t element_size);

// Free the dynamic array
void release_dynamic_array(DynamicArray *array);

// Marshal a message into a buffer
const char *marshal_message(const Message *msg);

// Unmarshal a message from a buffer
Message *unmarshal_message(const char *buffer);

// Marshal an uint64_t array into a buffer
char *marshal_uint64_array(DynamicArray *array);

// Unmarshal an uint64_t array from a buffer
DynamicArray *unmarshal_uint64_array(const char *buffer);

// Diff between two arrays
int diff_from_arrays(DynamicArray *first_array, DynamicArray *second_array, void* radio);

#endif //DYNAMIC_ARRAY_H