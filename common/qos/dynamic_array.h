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

#define MAX_MESSAGE_LENGTH 1024

typedef struct {
    uint64_t id;
    char content[MAX_MESSAGE_LENGTH];
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

extern DynamicArray awaiting_ack_msg_ids;

// Initialize dynamic array with a certain capacity
void init_dynamic_array(DynamicArray *array, size_t initial_capacity);

// Resize the dynamic array
void resize_dynamic_array(DynamicArray *array);

// Add an item to the dynamic array
void add_to_dynamic_array(DynamicArray *array, Message message);

// Debugging function to print the dynamic array
void print_dynamic_array(DynamicArray *array);

// Generate a unique message ID
uint64_t generate_unique_msg_id();

// Add a message ID to the array of IDs awaiting ACK
void add_msg_id_for_ack(Message message);

// Remove a message ID from the array of IDs awaiting ACK
void remove_msg_id(DynamicArray *array, uint64_t msg_id);

// Free the dynamic array
void release_dynamic_array(DynamicArray *array);


#endif //DYNAMIC_ARRAY_H