#include "fs_utils.h"
#include "core/logger.h"
#include "time_utils.h"
#include "core/config.h"
#include "qos/dynamic_array.h"
#include <json-c/json.h>


char *date_time = NULL;
static int file_counter = 0; // Counter for the file names
#define MAX_MESSAGES 100000 // Max messages per file
pthread_mutex_t json_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex for json_messages
json_object *g_json_messages = NULL; // Global JSON object to store the received messages

/**
 * @brief Save statistics to a file
 */
void save_stats_to_file(json_object **json_messages_ptr) {

    if (json_messages_ptr == NULL || *json_messages_ptr == NULL || json_object_array_length(*json_messages_ptr) == 0) {
        return;
    }

    pthread_mutex_lock(&json_mutex); // Lock the mutex

    json_object *json_messages = *json_messages_ptr; // Dereference the pointer for easier usage

    bool free_space = false;
    if (json_object_array_length(json_messages) >= MAX_MESSAGES) {
        // Get new file name
        file_counter++;
        free_space = true;
    }


    char *fullPath = create_stats_path();

    // Extract the folder path
    char *folder = strdup(fullPath); // Duplicate fullPath because dirname can modify the input argument
    char *dir = dirname(folder);

    free(folder); // Free the memory allocated by strdup

    create_if_not_exist_folder(dir);

    logger(LOG_LEVEL_DEBUG, "Saving statistics to file: %s", fullPath);

    // Todo check if the file exists with bool check_file_exists(char* fullPath) and if it exists, append to it instead of overwriting it

    FILE *file = fopen(fullPath, "w");
    free(fullPath);

    if (file == NULL) {
        logger(LOG_LEVEL_ERROR, "Impossibile aprire il file per la scrittura.");
        pthread_mutex_unlock(&json_mutex); // Unlock the mutex
        return;
    }

    if (config.use_json) {
        json_object *jobj = json_object_new_object();
        json_object_object_add(jobj, "messages", json_messages);

        const char *json_data = json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_PRETTY);
        fprintf(file, "%s\n", json_data);

        // Clear json_messages
        json_messages = json_object_new_array(); // Added to store all messages
    } else {
        // Write the CSV header
        fprintf(file, "id,num,diff\n");

        unsigned long array_len = json_object_array_length(json_messages);
        for (unsigned int i = 0; i < array_len; i++) {
            json_object *json_msg = json_object_array_get_idx(json_messages, i);

            uint64_t send_time = json_object_get_uint64(json_object_object_get(json_msg, "send_time"));
            uint64_t recv_time = json_object_get_uint64(json_object_object_get(json_msg, "recv_time"));

            double diff = (double) (recv_time - send_time);

            diff = diff / 1000; // Convert from nanoseconds to milliseconds

            if (json_msg) {
                fprintf(file, "%s,%d,%.4f\n",
                        json_object_get_string(json_object_object_get(json_msg, "id")),
                        i + 1,
                        diff
                );
            }
        }
    }
    fclose(file);

    if (free_space) {
        // Clean the json_messages array
        json_object_put(json_messages); // This will free the memory associated with json_messages
        *json_messages_ptr = json_object_new_array(); // Create a new array and update the original pointer
    }

    pthread_mutex_unlock(&json_mutex); // Unlock the mutex

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
 * @brief Check if a file exists
 * @param file_path
 * @return true if the file exists, otherwise false
 */
bool check_file_exists(char *file_path) {
    struct stat buffer;
    return (stat(file_path, &buffer) == 0);
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
            // +3 for the '/' and the null terminator + 2 for the '_' and the 'file_counter'
            + strlen(file_extension) + 3 + 2;

    // Allocate memory for the full file path
    char *fullPath = (char *) malloc(totalLength * sizeof(char));

    if (fullPath == NULL) {
        logger(LOG_LEVEL_ERROR, "Errore di allocazione della memoria.");
        free(date_time);
        return NULL;
    }

    // Construct the full file path
    snprintf(
            fullPath, totalLength, "%s/%s_%s_%d_result%s",
            config.stats_folder_path, date_time, config.protocol, file_counter, file_extension);

    return fullPath;
}

/**
 * @brief Free the memory allocated for the date_time variable
 */
void release_date_time() {
    free(date_time);
    date_time = NULL;
}

// ============================================= JSON Processing =======================================================

/**
 * @brief Initialize the global JSON object
 */
void init_json_messages() {
    if (g_json_messages == NULL) {
        g_json_messages = json_object_new_array();
    }
}

/**
 * @brief Free the global JSON object
 */
void release_json_messages() {
    json_object_put(g_json_messages);
    g_json_messages = NULL;
}

/**
 * @brief Add a new message to the global JSON object
 * @param json_str
 * @param recv_time
 */
void process_json_message(Message *msg) {
    long long recv_time = get_current_time_microseconds(); // Get the current time
    // Add the message to the statistical data

    // Create a JSON object for the message
    json_object *j_obj = json_object_new_object();
    json_object_object_add(j_obj, "id", json_object_new_uint64(msg->id));
    json_object_object_add(j_obj, "send_time", json_object_new_uint64(msg->timestamp));
    json_object_object_add(j_obj, "recv_time", json_object_new_uint64(recv_time));
    // json_object_object_add(j_obj, "message", json_object_new_string(msg->content));

    pthread_mutex_lock(&json_mutex);    // Import for not corrupting the json_messages array during the saving
    json_object_array_add(g_json_messages, j_obj);
    pthread_mutex_unlock(&json_mutex);
}

