#include "dynamic_array.h"
#include "core/logger.h"
#include "qos/interpolation_search.h"
#include <inttypes.h>

// Atomic for thread-safe unique message ID generation
volatile uint64_t atomic_msg_id = 0;

// Mutex for thread-safety while accessing the IDs array
pthread_mutex_t msg_ids_mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * @brief Initialize the dynamic array with the given initial capacity.
 * @param array
 * @param initial_capacity
 */
void init_dynamic_array(DynamicArray *array, size_t initial_capacity) {
    array->data = malloc(initial_capacity * sizeof(Message));
    array->capacity = initial_capacity;
    array->size = 0;
}

/**
 * @brief Resize the dynamic array by 50% of its current capacity.
 * @param array
 */
void resize_dynamic_array(DynamicArray *array) {
    size_t new_capacity = array->capacity + (array->capacity / 2); // 50% increase
    array->data = realloc(array->data, new_capacity * sizeof(Message));
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
 * @param message
 */
void add_message_to_dynamic_array(DynamicArray *array, Message message) {
    pthread_mutex_lock(&msg_ids_mutex);
    if (array->size == array->capacity) {
        resize_dynamic_array(array);
    }
    array->data[array->size].id = message.id;
    array->data[array->size].content = strdup(message.content); // Deep copy of content
    array->size++;
    pthread_mutex_unlock(&msg_ids_mutex);
}


/**
 * @brief Debugging function to print the dynamic array.
 * @param array
 */
void print_dynamic_array(DynamicArray *array) {
    printf("Dynamic array: ");
    for (size_t i = 0; i < array->size; i++) {
        printf("%" PRIu64 " " "%s", array->data[i].id, array->data[i].content);
    }
    printf("\n");
}

/**
 * @brief Create a message with the given content.
 * @param content
 * @return
 */
Message *create_message(const char *content) {
    if (content == NULL) {
        // Handle null pointer if necessary
        return NULL;
    }

    Message *msg = malloc(sizeof(Message));
    if (msg == NULL) {
        // Handle memory allocation failure if necessary
        return NULL;
    }
    size_t content_length = strlen(content);
    msg->id = generate_unique_message_id();

    // Allocate space for the content
    msg->content = malloc(content_length + 1);
    if (msg->content == NULL) {
        // Handle memory allocation failure if necessary
        return NULL;
    }
    strncpy(msg->content, content, content_length);
    msg->content[content_length] = '\0'; // Ensure null termination

    return msg;
}


/**
 * @brief Generate a unique message ID.
 * @return
 */
uint64_t generate_unique_message_id() {
    return __atomic_add_fetch(&atomic_msg_id, 1, __ATOMIC_SEQ_CST);
}

/**
 * @brief Remove a message ID from the array of IDs awaiting ACK.
 * @param array
 * @param msg_id
 */
void remove_message_by_id(DynamicArray *array, uint64_t msg_id, bool use_interpolation_search) {
    pthread_mutex_lock(&msg_ids_mutex);
    long long index = -1;

    if (use_interpolation_search) {
        index = interpolate_search(array, msg_id);
    }

    if (!use_interpolation_search || index == -1) {
        for (long long i = 0; i < array->size; i++) {
            if (array->data[i].id == msg_id) {
                index = i;
                break;
            }
        }
    }

    if (index != -1) {
        // Free the content of the message to be removed
        free(array->data[index].content);

        // Shift all elements after the found index to the left
        for (long long i = index; i < array->size - 1; i++) {
            array->data[i] = array->data[i + 1];
        }

        // Decrease the size of the array
        array->size--;
    }

    pthread_mutex_unlock(&msg_ids_mutex);
}

/**
 * @brief Release the resources allocated for a message.
 * @param msg
 */
void release_message(Message *msg) {
    if (msg->content != NULL) {
        free(msg->content);
        msg->content = NULL;
    }

    if (msg != NULL) {
        free(msg);
        msg = NULL;
    }
}


/**
 * @brief Free the dynamic array.
 * @param array
 */
void release_dynamic_array(DynamicArray *array) {
    for (size_t i = 0; i < array->size; i++) {
        free(array->data[i].content); // Free the content of each Message
    }
    free(array->data); // Free the array of Messages
    array->data = NULL;
    array->capacity = 0;
    array->size = 0;
}


/**
 * @brief Marshal a message into a buffer.
 * @param msg The message to marshal
 */
const char *marshal_message(const Message *msg) {
    // Calculate buffer size: sizeof(uint64_t) for id + string length + 1 for null terminator
    size_t buffer_size = sizeof(msg->id) + strlen(msg->content) + 1;

    // Allocate buffer
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        return NULL; // memory allocation failed
    }

    // Copy ID (consider endianness)
    uint64_t net_id = htonll(msg->id); // Convert to network byte order
    memcpy(buffer, &net_id, sizeof(net_id));

    // Copy content
    strcpy(buffer + sizeof(net_id), msg->content);

    return buffer; // return the allocated buffer
}

/**
 * @brief Unmarshal a message from a buffer.
 * @param buffer The buffer to unmarshal
 * @return
 */
Message *unmarshal_message(const char *buffer) {
    // Allocate space for Message
    Message *msg = malloc(sizeof(Message));
    if (msg == NULL) {
        return NULL; // memory allocation failed
    }

    // Extract ID (consider endianness)
    uint64_t net_id;
    memcpy(&net_id, buffer, sizeof(net_id));
    msg->id = ntohll(net_id); // Convert from network byte order

    size_t content_length = strlen(buffer + sizeof(net_id));
    msg->content = malloc(content_length + 1);
    if (msg->content == NULL) {
        return NULL; // memory allocation failed
    }

    // Extract content
    strncpy(msg->content, buffer + sizeof(net_id), content_length);
    msg->content[content_length] = '\0'; // Ensure null termination

    return msg; // return the allocated Message
}


/*
 *   // Usage of the Marshal and Unmarshal functions
 *
 *   Message *msg = create_message("Hello world");
 *   const char *buffer = marshal_message(msg);
 *
 *   Message *new_msg = unmarshal_message(buffer);
 *   printf("Message: %s\n", new_msg->content);
 *
 *   release_message(msg);
 *   release_message(new_msg);
 *   free((void *) buffer);
 */



