#include "dynamic_array.h"
#include "core/logger.h"
#include "qos/interpolation_search.h"
#include <inttypes.h>

// Atomic for thread-safe unique message ID generation
volatile uint64_t atomic_msg_id = 0;

// Mutex for thread-safety while accessing the IDs array
pthread_mutex_t msg_ids_mutex = PTHREAD_MUTEX_INITIALIZER;

DynamicArray g_array;


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

    // Deep Copy the element
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
 * @brief Get an element by its index. If negative, it will be counted from the end of the array.
 * @return The element at the given index, or NULL if the index is out of bounds.
 */
void *get_element_by_index(DynamicArray *array, long long index) {
    if (index < 0) {
        index = (long long) array->size + index;
    }

    if (index < 0 || index >= array->size) {
        return NULL;
    }

    return array->data[index];
}


/**
 * @brief Remove a message ID from the array of IDs awaiting ACK.
 * @param array
 * @param msg_id
 */
long long remove_element_by_id(DynamicArray *array, uint64_t msg_id, bool use_interpolation_search) {
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
    return index;
}

/**
 * @brief Clean all elements from the array.
 * @param array
 * @return
 */
size_t clean_all_elements(DynamicArray *array) {
    size_t count = 0;
    for (size_t i = 0; i < array->size; i++) {
        release_element(array->data[i], array->element_size);
        count++;
    }
    array->size = 0;
    return count;
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
        release_element(array->data[i], array->element_size);
    }
    free(array->data); // Free the array of Messages
    array->data = NULL;
    array->capacity = 0;
    array->size = 0;
    array->element_size = 0;
}

// ======================================= Marshalling and Unmarshalling =============================================//
/*
 * These functions are used to marshal and unmarshal messages struct and uint64_t arrays. Right now, they are not
 * efficient in terms of speed and memory usage, since they use snprintf and strtok_r to serialize and deserialize.
 * Given that they are used for testing purposes, this is not a big issue. However, if we want to use them in the
 * future, we should use a more efficient serialization method that can use Protobuf or Flatbuffers.
 */


/**
 * @brief Marshal a message into a buffer.
 * @param msg The message to marshal
 */
const char *marshal_message(const Message *msg) {
    if (msg == NULL) {
        return NULL;
    }

    // Estimate buffer size needed
    size_t buffer_size = snprintf(NULL, 0, "%" PRIu64 "|%s", msg->id, msg->content) + 1;
    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }

    snprintf(buffer, buffer_size, "%" PRIu64 "|%s", msg->id, msg->content);
    return buffer;
}


/**
 * @brief Unmarshal a message from a buffer.
 * @param buffer The buffer to unmarshal
 * @return
 */
Message *unmarshal_message(const char *buffer) {
    if (buffer == NULL) {
        return NULL;
    }

    Message *msg = malloc(sizeof(Message));
    if (msg == NULL) {
        return NULL;
    }

    uint64_t id;
    char *content = strdup(buffer);
    char *separator = strchr(content, '|');
    if (separator != NULL) {
        *separator = '\0';
        id = strtoull(content, NULL, 10);
        msg->content = strdup(separator + 1);
    } else {
        id = strtoull(content, NULL, 10);
        msg->content = NULL;
    }
    free(content);

    msg->id = id;
    return msg;
}

// I want that marshal_uint64_array give me back an array of a struct that contain maximum a char of 1024 because i can't send more than 1024 bytes in a buffer

/**
 * @brief Marshal a uint64_t array into a buffer.
 * @param array
 * @return
 */
char *marshal_uint64_array(DynamicArray *array) {
    if (array == NULL || array->size == 0) {
        return NULL;
    }

    // Calculate required buffer size
    size_t buffer_size = 0;
    for (size_t i = 0; i < array->size; i++) {
        buffer_size += snprintf(NULL, 0, "%" PRIu64 "|", *(uint64_t *) array->data[i]) + 1;
    }

    char *buffer = malloc(buffer_size);
    if (buffer == NULL) {
        return NULL;
    }

    char *ptr = buffer;
    for (size_t i = 0; i < array->size; i++) {
        ptr += sprintf(ptr, "%" PRIu64 "|", *(uint64_t *) array->data[i]);
    }
    *(ptr - 1) = '\0'; // Replace the last '|' with a null terminator

    return buffer;
}


/**
 * @brief Unmarshal a uint64_t array from a buffer.
 * @param buffer
 * @return
 */
DynamicArray *unmarshal_uint64_array(const char *buffer) {
    if (buffer == NULL) {
        return NULL;
    }

    DynamicArray *array = malloc(sizeof(DynamicArray));
    if (array == NULL) {
        return NULL;
    }
    init_dynamic_array(array, 10, sizeof(uint64_t));

    char *token;
    char *str = strdup(buffer);
    char *rest = str;

    while ((token = strtok_r(rest, "|", &rest))) {
        uint64_t value = strtoull(token, NULL, 10);
        add_to_dynamic_array(array, &value);
    }

    free(str);
    return array;
}

/**
 * @brief This function is used to get the difference between two arrays. It returns the number of missed messages.
 * @param first_array The array of the messages sent from the client to the server.
 * @param second_array The array of the messages received from the server.
 * @return The number of missed messages.
 */
int diff_from_arrays(DynamicArray *first_array, DynamicArray *second_array) {
    // Get the last message id from the array data.
    uint64_t *last_id = get_element_by_index(second_array, -1);     // new_array
    uint64_t *first_id = get_element_by_index(first_array, 0);      // g_array

    if (last_id == NULL || first_id == NULL) {
        return 0;
    }

    int missed_count = 0;

    // Remove from the low index to the last index of the array messages, if they are missing, count them.
    size_t idx = 0;
    size_t idx_start = *first_id;
    size_t idx_end = *last_id;
    for (size_t i = idx_start; i <= idx_end; i++) {
        if (remove_element_by_id(second_array, i, true) == -1) {
            missed_count++;
            // todo depends on the type of the array we could resend the message
        }
        idx++;

        // This is needed to clean the array of IDs from the client
        remove_element_by_id(first_array, i, true);
    }

    // This is needed to clean the array of IDs from the client
    //    clean_all_elements(first_array);

    return missed_count;

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





