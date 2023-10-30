#include "utils.h"
#include "core/config.h"
#include "core/logger.h"
#include "core/zhelpers.h"
#include "string_manip.h"

volatile sig_atomic_t interrupted = 0;


// List of IDs received since the last heartbeat
char **message_ids = NULL;
size_t num_message_ids = 0;
pthread_mutex_t message_ids_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect message_ids
const int MAX_RESPONSE_LENGTH = 1024;
char *IDS_SEPARATOR = ",";

// sizeof(double) could be 8, but It's string representation could be 20 characters
#define ID_MAX_LENGTH 37 // Maximum length of an ID (36 characters for double and 1 for '\0')

// ====================================== Message Manipulation =========================================================

/**
 * @brief Process the IDs received since the last heartbeat and send them back to the client
 * @param responder
 * @param last_id
 */
void process_message_ids(void *responder, char *last_id) {
    pthread_mutex_lock(&message_ids_mutex); // ensure thread safety

    if (num_message_ids > 0) {
        size_t id_with_separator_length = ID_MAX_LENGTH + strlen(IDS_SEPARATOR);
        size_t max_ids_per_message = (MAX_RESPONSE_LENGTH - strlen(IDS_SEPARATOR)) /
                                     id_with_separator_length; // minus one separator for last_id

        char *ids_string = malloc(MAX_RESPONSE_LENGTH);
        if (ids_string == NULL) {
            logger(LOG_LEVEL_ERROR, "Memory allocation error.");
            pthread_mutex_unlock(&message_ids_mutex);
            return;
        }

        size_t current_id_count = 0;
        ids_string[0] = '\0'; // Start with an empty string

        for (size_t i = 0; i < num_message_ids; ++i) {
            if (current_id_count > 0) {
                strcat(ids_string, IDS_SEPARATOR);
            }

            strcat(ids_string, message_ids[i]);
            current_id_count++;

            if (current_id_count == max_ids_per_message || (i == num_message_ids - 1 && last_id)) {
                if (i == num_message_ids - 1 && last_id) {
                    strcat(ids_string, IDS_SEPARATOR);
                    strcat(ids_string, last_id);
                }

                // Send the current string of IDs
                while (zmq_send_group(responder, get_group(RESPONDER_GROUP), ids_string, 0) < 0) {
                    if (errno == EAGAIN) {
                        logger(LOG_LEVEL_ERROR, "Timeout occurred while sending IDs.");
                        continue; // Or apply your desired logic for timeouts
                    } else {
                        logger(LOG_LEVEL_ERROR, "Error occurred while sending IDs.");
                        break; // Or handle other types of errors as you see fit
                    }
                }

//                logger(LOG_LEVEL_INFO, "Sent IDs.");

                ids_string[0] = '\0'; // Reset the string to empty
                current_id_count = 0; // Reset the ID counter
            }

            free(message_ids[i]); // free the memory allocated for this ID
        }

        if (ids_string[0] != '\0' && current_id_count > 0) {
            // Send remaining IDs
            while (zmq_send_group(responder, get_group(RESPONDER_GROUP), ids_string, 0) < 0) {
                if (errno == EAGAIN) {
                    logger(LOG_LEVEL_ERROR, "Timeout occurred while sending IDs.");
                    continue; // Or apply your desired logic for timeouts
                } else {
                    logger(LOG_LEVEL_ERROR, "Error occurred while sending IDs.");
                    break; // Or handle other types of errors as you see fit
                }
            }

            logger(LOG_LEVEL_INFO, "Sent IDs.");
        }

        free(ids_string); // free the buffer
        free(message_ids); // free the array itself
        message_ids = NULL; // reset the pointer
        num_message_ids = 0; // reset the counter
    }

    pthread_mutex_unlock(&message_ids_mutex); // Unlock the mutex
}


/**
 * @brief Get the message ID at the given index (negative index means from the end)
 * @param index
 * @return char*
 */
char *get_message_id(int index) {
    char *msg_id = NULL;

    pthread_mutex_lock(&message_ids_mutex); // ensure thread safety

    if (num_message_ids == 0) {
        // There are no messages in the array.
        pthread_mutex_unlock(&message_ids_mutex);
        return NULL; // Or handle it as appropriate for your program
    }

    // Check for potential underflow.
    if (index < 0 && (size_t) (-index) > num_message_ids) {
        // The index is too negative and would underflow.
        pthread_mutex_unlock(&message_ids_mutex);
        return NULL; // Or handle it as appropriate for your program
    }

    size_t adjusted_index;
    if (index < 0) {
        // Safe because we already checked for underflow.
        adjusted_index = num_message_ids - (size_t) (-index);
    } else {
        adjusted_index = (size_t) index;
    }

    // Check if the index is within bounds.
    if (adjusted_index < num_message_ids) {
        msg_id = message_ids[adjusted_index];
    } else {
        // Index is out of bounds.
        msg_id = NULL; // Or handle it as appropriate for your program
    }

    pthread_mutex_unlock(&message_ids_mutex); // Unlock the mutex
    return msg_id;
}


/**
 * @brief Add a message ID to the list of IDs received since the last heartbeat
 * @param id_str
 */
void add_message_id(const char *id_str) {
    pthread_mutex_lock(&message_ids_mutex); // ensure thread safety
    message_ids = realloc(message_ids, sizeof(char *) * (num_message_ids + 1)); // expand the array
    if (message_ids == NULL) {
        logger(LOG_LEVEL_ERROR, "Errore di allocazione della memoria.");
        return;
    }
    message_ids[num_message_ids] = strdup(id_str); // store a copy of the ID
    num_message_ids++;
    pthread_mutex_unlock(&message_ids_mutex);
}


/**
 * @brief Process the IDs received since the last heartbeat and return the missed ones.
 * @param buffer The buffer containing IDs separated by commas, with the last one being the last acknowledged ID.
 * @param missed_count A pointer to store the count of missed message IDs.
 * @return char** Array of missed message IDs.
 */
char **process_missed_message_ids(const char *buffer, size_t *missed_count) {
    pthread_mutex_lock(&message_ids_mutex); // Ensure thread safety while accessing shared data.

    // Allocate memory for missed IDs.
    char **missed_ids = NULL;
    *missed_count = 0;

    size_t num_ids_in_buffer;
    char **ids_in_buffer = split_string(buffer, IDS_SEPARATOR[0], &num_ids_in_buffer);

    if (!ids_in_buffer || num_ids_in_buffer == 0) {
        logger(LOG_LEVEL_ERROR, "Error: No IDs found in the buffer.");
        pthread_mutex_unlock(&message_ids_mutex);
        return NULL; // No IDs found in the buffer, return NULL.
    }

    // The last ID in the buffer is the last_id.
    char *last_id = ids_in_buffer[num_ids_in_buffer - 1];

    size_t last_id_index_in_message_ids = -1;
    // Find the index of the last_id in the message_ids array.
    for (size_t i = 0; i < num_message_ids; ++i) {
        if (strcmp(message_ids[i], last_id) == 0) {
            last_id_index_in_message_ids = i;
            break;
        }
    }

    if (last_id_index_in_message_ids == -1) {
        logger(LOG_LEVEL_ERROR, "Last ID not found in message IDs; all IDs missed.");
        *missed_count = num_message_ids;
        pthread_mutex_unlock(&message_ids_mutex);
        return message_ids; // All IDs missed, return all message_ids.
    }

    // Identify missed IDs and reallocate missed_ids array accordingly.
    for (size_t i = 0; i <= last_id_index_in_message_ids; ++i) {
        bool found = false;
        for (size_t j = 0; j < num_ids_in_buffer; ++j) {
            if (strcmp(message_ids[i], ids_in_buffer[j]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            ++(*missed_count);
            missed_ids = (char **) realloc(missed_ids, (*missed_count) * sizeof(char *));
            if (!missed_ids) {
                logger(LOG_LEVEL_ERROR, "Memory allocation failure for missed_ids.");
                *missed_count = 0;
                break; // In case of memory allocation failure, break the loop.
            } else {
                missed_ids[(*missed_count) - 1] = strdup(message_ids[i]);
            }
        }
    }

    // Free the memory of message IDs that are up to the last acknowledged ID.
    for (size_t i = 0; i <= last_id_index_in_message_ids; ++i) {
        free(message_ids[i]);
        message_ids[i] = NULL;
    }

    // Shift the remaining IDs down in the array.
    size_t new_num_message_ids = num_message_ids - (last_id_index_in_message_ids + 1);
    for (size_t i = 0; i < new_num_message_ids; ++i) {
        message_ids[i] = message_ids[i + last_id_index_in_message_ids + 1];
    }

    // Attempt to shrink the memory used by the message_ids array.
    char **new_message_ids = realloc(message_ids, new_num_message_ids * sizeof(char *));
    if (!new_message_ids) {
        logger(LOG_LEVEL_ERROR, "Memory reallocation failure for message_ids.");
    } else {
        message_ids = new_message_ids;
    }
    num_message_ids = new_num_message_ids;

    // Free the split IDs from the buffer.
    for (size_t i = 0; i < num_ids_in_buffer; ++i) {
        free(ids_in_buffer[i]);
    }
    free(ids_in_buffer);

    pthread_mutex_unlock(&message_ids_mutex); // Unlock the mutex after processing.

    return missed_ids; // Return the array of missed IDs.
}

/**
 * Example of usage Message Manipulation
    - Client responder thread:

    size_t missed_count = 0;
    char **missed_ids = process_missed_message_ids(buffer, &missed_count);

    - Client publisher thread:

    add_message_id(msg_id);

    - Server thread:

    if (strncmp(buffer, "HB", 2) == 0)
        process_message_ids(radio, last_id);

    add_message_id(buffer);
 */


// ============================================= Signal Handling =======================================================


/**
 * @brief Handle the keyboard interruption (Ctrl+C)
 * @param signal
 */
void handle_interrupt(int sig) {
    (void) sig; // Avoid unused parameter warning
    interrupted = 1;
    g_linger_timeout = 0; // Set the linger timeout to 0 to force the socket to close immediately
    printf("\n[WARNING]: Interruzione da tastiera ricevuta (Ctrl+C)\n");
}
