#include "fs_utils.h"
#include "core/logger.h"
#include "time_utils.h"
#include "core/config.h"


char *date_time = NULL;

/**
 * @brief Save statistics to a file
 */
void save_stats_to_file(json_object *json_messages) {
    if (json_messages == NULL || json_object_array_length(json_messages) == 0) {
        return;
    }

    char *fullPath = create_stats_path();

    // Extract the folder path
    char *folder = strdup(fullPath); // Duplicate fullPath because dirname can modify the input argument
    char *dir = dirname(folder);
    create_if_not_exist_folder(dir);

    logger(LOG_LEVEL_DEBUG, "Saving statistics to file: %s", fullPath);

    FILE *file = fopen(fullPath, "w");
    free(fullPath);

    if (file == NULL) {
        logger(LOG_LEVEL_ERROR, "Impossibile aprire il file per la scrittura.");
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

            double diff = json_object_get_double(json_object_object_get(json_msg, "recv_time")) -
                          json_object_get_double(json_object_object_get(json_msg, "send_time"));
            if (json_msg) {
                fprintf(file, "%s,%d,%f\n",
                        json_object_get_string(json_object_object_get(json_msg, "id")),
                        i + 1,
                        diff
                );
            }
        }
    }
    fclose(file);
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

