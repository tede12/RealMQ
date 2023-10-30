#include "dynamic_array.h"
#include "core/logger.h"

// Atomic for thread-safe unique message ID generation
volatile uint64_t atomic_msg_id = 0;

// Mutex for thread-safety while accessing the IDs array
pthread_mutex_t msg_ids_mutex = PTHREAD_MUTEX_INITIALIZER;

DynamicArray awaiting_ack_msg_ids;

/**
 * @brief Initialize the dynamic array with the given initial capacity.
 * @param array
 * @param initial_capacity
 */
void init_dynamic_array(DynamicArray *array, size_t initial_capacity) {
    array->data = malloc(initial_capacity * sizeof(uint64_t));
    array->capacity = initial_capacity;
    array->size = 0;
}

/**
 * @brief Resize the dynamic array by 50% of its current capacity.
 * @param array
 */
void resize_dynamic_array(DynamicArray *array) {
    size_t new_capacity = array->capacity + (array->capacity / 2); // 50% increase
    array->data = realloc(array->data, new_capacity * sizeof(uint64_t));
    // fixme if realloc fails, we lose the data in the array (should be handled in some way)
    if (array->data == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to resize dynamic array");
        exit(EXIT_FAILURE);
    }
    array->capacity = new_capacity;
}

/**
 * @brief Add an item to the dynamic array.
 * @param array
 * @param value
 */
void add_to_dynamic_array(DynamicArray *array, uint64_t value) {
    if (array->size == array->capacity) {
        resize_dynamic_array(array);
    }
    array->data[array->size] = value;
    array->size++;
}

/**
 * @brief Debugging function to print the dynamic array.
 * @param array
 */
void print_dynamic_array(DynamicArray *array) {
    printf("Dynamic array: ");
    for (size_t i = 0; i < array->size; i++) {
        printf("%llu ", array->data[i]);
    }
    printf("\n");
}

/**
 * @brief Generate a unique message ID.
 * @return
 */
uint64_t generate_unique_msg_id() {
    return __atomic_add_fetch(&atomic_msg_id, 1, __ATOMIC_SEQ_CST);
}

/**
 * @brief Add a message ID to the array of IDs awaiting ACK.
 * @param msg_id
 */
void add_msg_id_for_ack(uint64_t msg_id) {
    pthread_mutex_lock(&msg_ids_mutex);
    add_to_dynamic_array(&awaiting_ack_msg_ids, msg_id);
    pthread_mutex_unlock(&msg_ids_mutex);
}

/**
 * @brief Remove a message ID from the array of IDs awaiting ACK.
 * @param array
 * @param msg_id
 */
void remove_msg_id(DynamicArray *array, uint64_t msg_id) {
    pthread_mutex_lock(&msg_ids_mutex);
    for (size_t i = 0; i < array->size; i++) {
        if (array->data[i] == msg_id) {
            for (size_t j = i; j < array->size - 1; j++) {
                array->data[j] = array->data[j + 1];
            }
            array->size--;
            break;
        }
    }
    pthread_mutex_unlock(&msg_ids_mutex);
}

/**
 * @brief Free the dynamic array.
 * @param array
 */
void release_dynamic_array(DynamicArray *array) {
    free(array->data);
    array->data = NULL;
    array->capacity = 0;
    array->size = 0;
}

