#include "utils.h"
#include "config.h"
#include "logger.h"
#include "zhelpers.h"

volatile sig_atomic_t interrupted = 0;
char *date_time = NULL;

// List of IDs received since the last heartbeat
char **message_ids = NULL;
size_t num_message_ids = 0;
pthread_mutex_t message_ids_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex to protect message_ids
const int MAX_RESPONSE_LENGTH = 1024;
char *IDS_SEPARATOR = ",";

// sizeof(double) could be 8, but It's string representation could be 20 characters
#define ID_MAX_LENGTH 21 // Maximum length of an ID (20 characters for double and 1 for '\0')

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

                logger(LOG_LEVEL_INFO, "Sent IDs.");

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
 * @brief Splits a string by a delimiter and returns an array of tokens.
 * @param str The string to split.
 * @param delim The delimiter character.
 * @param num_tokens Output parameter to hold the number of tokens.
 * @return char** A dynamically-allocated array of strings. Free each element and the array itself after use.
 */
char **split_string(const char *str, const char delim, size_t *num_tokens) {
    *num_tokens = 0;

    // First, count the number of tokens.
    const char *ptr = str;
    while (*ptr) {
        if (*ptr == delim) {
            (*num_tokens)++;
        }
        ptr++;
    }
    (*num_tokens)++; // One more for the last token.

    // Allocate memory for tokens.
    char **tokens = malloc(*num_tokens * sizeof(char *));
    if (!tokens) {
        perror("Memory allocation error");
        *num_tokens = 0;
        return NULL;
    }

    size_t token_len = 0;
    ptr = str; // Reset pointer to the start of the string.
    for (size_t i = 0; i < *num_tokens; i++) {
        const char *start = ptr; // Start of the token.
        while (*ptr && *ptr != delim) {
            token_len++;
            ptr++;
        }

        tokens[i] = strndup(start, token_len);
        if (!tokens[i]) {
            perror("Memory allocation error");
            // Free already allocated memory and reset counter.
            for (size_t j = 0; j < i; j++) {
                free(tokens[j]);
            }
            free(tokens);
            *num_tokens = 0;
            return NULL;
        }

        token_len = 0; // Reset for the next token.
        if (*ptr) {
            ptr++; // Skip the delimiter.
        }
    }

    return tokens;
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
        fprintf(stderr, "Error: No IDs found in the buffer.\n");
    } else {
        // The last ID in the buffer is the last_id.
        char *last_id = ids_in_buffer[num_ids_in_buffer - 1];

        for (size_t i = 0; i < num_message_ids; ++i) {
            if (strcmp(message_ids[i], last_id) >
                0) { // If the current ID is "greater" than the last_id, it's considered "missed".
                ++(*missed_count);
                missed_ids = (char **) realloc(missed_ids, (*missed_count) * sizeof(char *));
                if (!missed_ids) {
                    perror("Memory allocation error");
                    *missed_count = 0;
                    break; // Break out of the loop in case of memory allocation failure.
                } else {
                    missed_ids[(*missed_count) - 1] = strdup(message_ids[i]);
                }
            }
        }

        // Free the original message_ids after processing.
        for (size_t i = 0; i < num_message_ids; ++i) {
            free(message_ids[i]);
        }
        free(message_ids);
        message_ids = NULL;
        num_message_ids = 0;

        // Free the split IDs from the buffer.
        for (size_t i = 0; i < num_ids_in_buffer; ++i) {
            free(ids_in_buffer[i]);
        }
        free(ids_in_buffer);
    }
//    missed_count--;

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


// ============================================= Time Functions ========================================================


/**
 * @brief Handle the keyboard interruption (Ctrl+C)
 * @param signal
 */
void handle_interrupt(int sig) {
    (void) sig; // Avoid unused parameter warning
    interrupted = 1;
    printf("\n[WARNING]: Interruzione da tastiera ricevuta (Ctrl+C)\n");
}

/**
 * @param folder_path
 * @return <folder_path> if the folder exists, otherwise create the folder and return <folder_path>
 */
char *create_if_not_exist_folder(char *folder_path) {
    struct stat st = {0};
    if (stat(folder_path, &st) == -1) {
        mkdir(folder_path, 0700);
    }
    return folder_path;
}

/**
 * @brief Create the full path for the statistics file
 * @param date_time
 * @return <config.stats_folder_path>/<date_time>_<config.protocol>_result.<config.stats_file_extension>
 */
char *create_stats_path() {
    // Generate one unique date time for each run
    if (date_time == NULL) {
        // Get the date + time for the filename
        date_time = malloc(20 * sizeof(char));
        if (date_time == NULL) {
            logger(LOG_LEVEL_ERROR, "Allocation error for date_time variable.");
            return NULL;
        }
        date_time = get_current_date_time();
    }

    // Get the file extension
    char *file_extension = config.use_json ? ".json" : ".csv";  // Added the dot (.) before the extensions

    // Calculate the total length of the final string
    // Lengths of the folder path, date_time, file extension, and additional characters
    // ("/", "_result.", and the null terminator)
    unsigned int totalLength =
            strlen(config.stats_folder_path)
            + strlen(date_time)
            + strlen(config.protocol)
            + strlen("_result")
            + strlen(file_extension) + 3;  // +3 for the '/' and the null terminator

    // Allocate memory for the full file path
    char *fullPath = (char *) malloc(totalLength * sizeof(char));

    if (fullPath == NULL) {
        logger(LOG_LEVEL_ERROR, "Errore di allocazione della memoria.");
        return NULL;
    }

    // Construct the full file path
    snprintf(
            fullPath, totalLength, "%s/%s_%s_result%s",
            config.stats_folder_path, date_time, config.protocol, file_extension);
    return fullPath;
}


/**
 * @brief Get the current timestamp (precision: seconds)
 *
 * @return time_t
 */
time_t get_timestamp() {
    time_t start_time;
    time(&start_time);
    return start_time;
}

/**
 * @brief Get the current date and time
 * @return char*
 */
char *get_current_date_time() {
    time_t rawtime;
    struct tm *timeinfo;
    char *buffer = (char *) malloc(80 * sizeof(char));

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer, 80, "%d_%m_%Y_%I_%M_%S", timeinfo);
    return buffer;
}


/**
 * @brief Get the current timestamp (precision: milliseconds)
 *
 * @return long long
 */
long long get_current_time_millis() {
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long) (ts.tv_sec) * 1000 + (long long) (ts.tv_nsec) / 1000000;
}

/**
 * @brief Get the current timestamp (precision: nanoseconds)
 *
 * @return long long
 */
long long get_current_time_nanos() {
    timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (long long) (ts.tv_sec) * 1000000000 + (long long) (ts.tv_nsec);
}


/**
 * @brief Get the current time object
 * @return timespec
 */
timespec getCurrentTime() {
    timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        perror("clock_gettime");
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        printf("[getCurrentTime][Error]: getting current time\n");
    }
    return ts;
}

/**
 * @brief Get the current time value object
 * @param ts pass NULL to get current time value or pass a timespec struct to get the time value of that struct
 * @return double
 */
double getCurrentTimeValue(timespec *ts) {
    timespec localTs;

    if (ts == NULL) {
        localTs = getCurrentTime();
        ts = &localTs;
    }

    double timeValue = (double) ts->tv_sec + (double) ts->tv_nsec / 1e9;
    return timeValue;
}


/**
 * @brief Get the elapsed time object
 * @param start is a timespec struct
 * @param end pass NULL to get current time value or pass a timespec struct to get the time value of that struct
 * @return double
 */
double getElapsedTime(timespec start, timespec *end) {
    timespec localEnd;
    if (end == NULL) {
        localEnd = getCurrentTime();
        end = &localEnd;
    }
    return (double) (end->tv_sec - start.tv_sec) + (double) (end->tv_nsec - start.tv_nsec) / 1e9;
}

/**
 * Example of usage:
    time_t start_time = get_timestamp();
    long long start_time2 = get_current_time_nanos();
    printf("Timestamp now: %ld, %lld\n", start_time, start_time2);

    sleep(5);

    time_t end_time = get_timestamp();
    long long end_time2 = get_current_time_nanos();
    printf("Timestamp now: %ld, %lld\n", end_time, end_time2);

    printf("Time difference: %.3f, %.3f\n", difftime(end_time, start_time), difftime(end_time2, start_time2) / 1000.0);
*/
