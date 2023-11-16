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
void init_dynamic_array(DynamicArray *array, size_t initial_capacity, size_t element_size) {
    array->data = malloc(initial_capacity * element_size);
    if (array->data == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory for dynamic array");
        exit(EXIT_FAILURE);
    }
    array->capacity = initial_capacity;
    array->size = 0;
    array->element_size = element_size;
}

/**
 * @brief Resize the dynamic array by 50% of its current capacity.
 * @param array
 */
void resize_dynamic_array(DynamicArray *array) {
    size_t new_capacity = array->capacity + (array->capacity / 2); // 50% increase
    array->data = realloc(array->data, new_capacity * array->element_size);
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
void add_to_dynamic_array(DynamicArray *array, void *element) {
    pthread_mutex_lock(&msg_ids_mutex);

    if (array->size == array->capacity) {
        resize_dynamic_array(array);
    }

    // Deep Copy

    void *new_element = NULL;
    if (array->element_size == sizeof(uint64_t)) {

        new_element = malloc(sizeof(uint64_t));
        if (new_element == NULL) {
            logger(LOG_LEVEL_ERROR, "Failed to allocate memory for uint64_t element");
            exit(EXIT_FAILURE);
        }
        memcpy(new_element, element, sizeof(uint64_t));
    } else if (array->element_size == sizeof(Message)) {
        new_element = malloc(sizeof(Message));
        if (new_element == NULL) {
            logger(LOG_LEVEL_ERROR, "Failed to allocate memory for Message element");
            exit(EXIT_FAILURE);
        }
        memcpy(new_element, element, sizeof(Message));

        // Duplicate the content string
        Message *original_msg = (Message *) element;
        Message *new_msg = (Message *) new_element;
        new_msg->content = strdup(original_msg->content);
        if (new_msg->content == NULL) {
            logger(LOG_LEVEL_ERROR, "Failed to allocate memory for message content");
            free(new_element);
            exit(EXIT_FAILURE);
        }
    } else {
        logger(LOG_LEVEL_ERROR, "Unsupported element size");
        exit(EXIT_FAILURE);
    }

    array->data[array->size++] = new_element;

    pthread_mutex_unlock(&msg_ids_mutex);
}


/**
 * @brief Debugging function to print the dynamic array.
 * @param array
 */
void print_dynamic_array(DynamicArray *array) {
    printf("Dynamic array: ");
    for (size_t i = 0; i < array->size; i++) {
        if (array->element_size == sizeof(Message)) {
            printf("%" PRIu64 " %s ", ((Message *) array->data[i])->id, ((Message *) array->data[i])->content);
        } else if (array->element_size == sizeof(uint64_t)) {
            printf("%" PRIu64 " ", *(uint64_t *) array->data[i]);
        }
    }
    printf("\n");
}

/**
 * @brief Create a message with the given content.
 * @param content
 * @return
 */
void *create_element(const char *content) {
    uint64_t msg_id = generate_unique_message_id();

    if (content == NULL) {
        uint64_t *p_msg_id = malloc(sizeof(uint64_t));
        if (p_msg_id == NULL) {
            logger(LOG_LEVEL_ERROR, "Failed to allocate memory for message ID");
            return NULL;
        }

        *p_msg_id = msg_id;
        return p_msg_id;
    }

    // Case of using Message struct
    Message *msg = malloc(sizeof(Message));
    if (msg == NULL) {
        // Handle memory allocation failure if necessary
        return NULL;
    }
    size_t content_length = strlen(content);
    msg->id = msg_id;

    // Allocate space for the content
    msg->content = malloc(content_length + 1);
    if (msg->content == NULL) {
        // Handle memory allocation failure if necessary
        free(msg);
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
void remove_element_by_id(DynamicArray *array, uint64_t msg_id, bool use_interpolation_search) {
    pthread_mutex_lock(&msg_ids_mutex);
    long long index = -1;

    if (use_interpolation_search) {
        index = interpolate_search(array, msg_id);
    }

    if (!use_interpolation_search || index == -1) {
        for (long long i = 0; i < array->size; i++) {
            if (array->element_size == sizeof(Message)) {
                if (((Message *) array->data[i])->id == msg_id) {
                    index = i;
                    break;
                }
            } else if (array->element_size == sizeof(uint64_t)) {
                if (*(uint64_t *) array->data[i] == msg_id) {
                    index = i;
                    break;
                }
            }
        }
    }

    if (index != -1) {

        // Release the resources allocated for the element
        release_element(array->data[index], array->element_size);

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
 * @brief Release the resources allocated for an element.
 * @param element
 */
void release_element(void *element, size_t element_size) {
    if (element_size == sizeof(Message)) {

        if (((Message *) element)->content != NULL) {
            free(((Message *) element)->content);
            ((Message *) element)->content = NULL;
        }

    }
    if (element != NULL) {
        free(element);
        element = NULL;
    }
}


/**
 * @brief Free the dynamic array.
 * @param array
 */
void release_dynamic_array(DynamicArray *array) {
    for (size_t i = 0; i < array->size; i++) {
//        if (array->element_size == sizeof(Message)) {
//            free(array->data[i]); // Free the Message
//        } else if (array->element_size == sizeof(uint64_t)) {
//            free(array->data[i]); // Free the ID
//        } else {
//            logger(LOG_LEVEL_ERROR, "Unsupported element size for dynamic array");
//            exit(EXIT_FAILURE);
//        }
        release_element(array->data[i], array->element_size);
    }
    free(array->data); // Free the array of Messages
    array->data = NULL;
    array->capacity = 0;
    array->size = 0;
    array->element_size = 0;
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
 *   Message *msg = create_element("Hello world");
 *   const char *buffer = marshal_message(msg);
 *
 *   Message *new_msg = unmarshal_message(buffer);
 *   printf("Message: %s\n", new_msg->content);
 *
 *   release_element(msg, sizeof(Message));
 *   release_element(new_msg, sizeof(Message));
 *   free((void *) buffer);
 */


/*
 *   // Usage of the Dynamic Array functions
 *   DynamicArray g_array;
 *   init_dynamic_array(&g_array, 5, sizeof(uint64_t));
 *
 *   Element *el = create_element(NULL);
 *   add_to_dynamic_array(&g_array, el);
 *
 *   release_element(el, sizeof(uint64_t));
 *   release_dynamic_array(&g_array);
 */





