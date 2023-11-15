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
    Message *data;  // Array of Message structures
    size_t size;    // Number of elements in the array
    size_t capacity; // Total capacity of the array
} DynamicArray;

// Initialize dynamic array with a certain capacity
void init_dynamic_array(DynamicArray *array, size_t initial_capacity);

// Resize the dynamic array
void resize_dynamic_array(DynamicArray *array);

// Add an item to the dynamic array
void add_message_to_dynamic_array(DynamicArray *array, Message message);

// Create a new message
Message *create_message(const char *content);

// Debugging function to print the dynamic array
void print_dynamic_array(DynamicArray *array);

// Generate a unique message ID
uint64_t generate_unique_message_id();

// Remove a message from the array
void remove_message_by_id(DynamicArray *array, uint64_t msg_id, bool use_interpolation_search);

// Free a message
void release_message(Message *msg);

// Free the dynamic array
void release_dynamic_array(DynamicArray *array);

// Marshal a message into a buffer
const char *marshal_message(const Message *msg);

// Unmarshal a message from a buffer
Message *unmarshal_message(const char *buffer);

#endif //DYNAMIC_ARRAY_H