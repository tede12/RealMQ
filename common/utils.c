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


/**
 * @brief Process the IDs received since the last heartbeat and send them back to the client
 * @param responder
 * @param last_id
 */
void process_message_ids(void *responder, char *last_id) {
    pthread_mutex_lock(&message_ids_mutex); // ensure thread safety

    if (num_message_ids > 0) {
        size_t current_size = 0;
        char *ids_string = malloc(MAX_RESPONSE_LENGTH); // buffer for each message
        if (ids_string == NULL) {
            logger(LOG_LEVEL_ERROR, "Errore di allocazione della memoria.");
            return;
        }
        ids_string[0] = '\0'; // start with an empty string

        for (size_t i = 0; i < num_message_ids; ++i) {
            current_size += strlen(message_ids[i]) + strlen(IDS_SEPARATOR); // +1 for separator

            // Check if adding the next ID would exceed the max length
            if (current_size + ((last_id != NULL) ? strlen(last_id) : 0) >= MAX_RESPONSE_LENGTH) {
                // Send the current string of IDs before it gets too large
                while (zmq_send_group(responder, get_group(RESPONDER_GROUP), ids_string, strlen(ids_string)) < 0) {
                    if (errno == EAGAIN) {
                        logger(LOG_LEVEL_ERROR, "Timeout occurred while sending IDs.");
                        continue;
                    } else {
                        logger(LOG_LEVEL_ERROR, "Error occurred while sending IDs.");
                        break;
                    }
                }
                logger(LOG_LEVEL_INFO, "Sent IDs.");
                ids_string[0] = '\0'; // Reset the string to empty
                current_size = 0; // Reset the size counter
            }

            // Append the ID and a separator to the string
            strcat(ids_string, message_ids[i]);
            strcat(ids_string, IDS_SEPARATOR);
            free(message_ids[i]); // free the memory allocated for this ID
        }

        // Append the last_id if it's provided and there's enough space
        if (last_id != NULL && current_size + strlen(last_id) < MAX_RESPONSE_LENGTH - 256) {
            strcat(ids_string, last_id);
            strcat(ids_string, IDS_SEPARATOR);
        }

        // Send any remaining IDs that didn't fill up the last message
        if (ids_string[0] != '\0') {
            while (zmq_send_group(responder, get_group(RESPONDER_GROUP), ids_string, strlen(ids_string)) < 0) {
                if (errno == EAGAIN) {
                    logger(LOG_LEVEL_ERROR, "Timeout occurred while sending IDs.");
                    continue;
                } else {
                    logger(LOG_LEVEL_ERROR, "Error occurred while sending IDs.");
                    break;
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
 * @brief Delete message IDs from the list of IDs received since the last heartbeat
 * @param response the response from the server (comma-separated list of IDs)
 */
void delete_message_ids_from_buffer(char *response) {
    assert(response != NULL); // Ensure response is valid

    pthread_mutex_lock(&message_ids_mutex); // Lock the mutex

    // Find the position of the last_id in the response
    char *last_id_start = strrchr(response, IDS_SEPARATOR[0]); // Find the last comma in the response
    if (!last_id_start) {
        // Handle the case where there's no comma in the response (i.e., one ID or none)
        last_id_start = response;
    } else {
        last_id_start++; // Move past the comma
    }

    // Copy last_id into a separate buffer since strtok will modify the original string
    char last_id[MAX_RESPONSE_LENGTH];
    strncpy(last_id, last_id_start, sizeof(last_id));
    last_id[sizeof(last_id) - 1] = '\0'; // Ensure null termination

    // Now, we split the response and process each ID until we reach last_id
    bool reached_last_id = false;
    int remaining_ids_index = 0; // Index for appending remaining IDs
    char *token = strtok(response, IDS_SEPARATOR);
    while (token != NULL && !reached_last_id) {
        if (strcmp(token, last_id) == 0) {
            reached_last_id = true; // We've found the last_id, so we stop after this
        } else {
            // If this ID is not the last_id, we need to check if it's in the global list
            bool found = false;
            for (int i = 0; i < num_message_ids; ++i) {
                if (strcmp(message_ids[i], token) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                // If the ID is not in the list, we keep it
                message_ids[remaining_ids_index++] = strdup(token);
            }
        }

        token = strtok(NULL, IDS_SEPARATOR); // Get the next token
    }

    // Now, append the IDs after the last_id from the original message_ids list
    for (int i = 0; i < num_message_ids; ++i) {
        if (strcmp(message_ids[i], last_id) > 0) { // Check if this ID is after the last_id
            message_ids[remaining_ids_index++] = strdup(message_ids[i]);
        }
        free(message_ids[i]); // Free each string since we've duplicated the ones we're keeping
    }

    // Update the count of message IDs
    num_message_ids = remaining_ids_index;

    pthread_mutex_unlock(&message_ids_mutex); // Unlock the mutex
}


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
