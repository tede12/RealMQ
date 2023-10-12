#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <yaml.h>
#include <stdlib.h>  // for strtol and free


typedef struct ActionType {
    char *name;
    int sleep_starting_time;
} ActionType;

/**
 * The configuration struct.
 */
typedef struct Config {
    char *address;
    int num_threads;
    int num_messages;
    bool use_json;
    bool use_msg_per_time;
    int msg_per_minute;
    int save_interval_seconds;
    int message_size;
    char *stats_filepath;
    ActionType *client_action;
    ActionType *server_action;
} Config;

/**
 * Get the next value from the YAML parser.
 * @param parser The YAML parser.
 * @return The next value, or NULL if there is no next value.
 */
char *next_value(yaml_parser_t *parser) {
    yaml_event_t event;
    if (!yaml_parser_parse(parser, &event) || event.type != YAML_SCALAR_EVENT) {
        yaml_event_delete(&event);
        return NULL;
    }
    char *value = strdup((char *) event.data.scalar.value);
    yaml_event_delete(&event);
    return value;
}

/**
 * Convert a string to an integer.
 * @param value The string to convert.
 * @return The integer value of the string, or -1 if the string is not a valid integer.
 */
int convert_to_int(const char *value) {
    char *endptr;
    errno = 0;
    long num = strtol(value, &endptr, 10);
    if (*endptr != '\0' || errno == ERANGE) {
        fprintf(stderr, "Conversion error or out of range: %s\n", value);
        return -1;
    }
    return (int) num;
}

/**
 * Read the configuration from a YAML file.
 * @param filename The name of the YAML file.
 * @param config The configuration struct to populate.
 * @return 0 if the configuration was read successfully, -1 otherwise.
 */
int read_config(const char *filename, Config *config) {
    FILE *fh = fopen(filename, "r");
    if (!fh) {
        fprintf(stderr, "Failed to open %s: %s\n", filename, strerror(errno));
        return -1;
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        fprintf(stderr, "Failed to initialize the YAML parser.\n");
        return -1;
    }
    yaml_parser_set_input_file(&parser, fh);

    // Allocate and initialize
    config->client_action = malloc(sizeof(ActionType));
    config->server_action = malloc(sizeof(ActionType));
    if (!config->client_action || !config->server_action) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return -1;
    }

    // Default values
    config->client_action->name = strdup("CLIENT");
    config->server_action->name = strdup("SERVER");

    yaml_event_t event;
    char *key = NULL, *value = NULL;
    while (yaml_parser_parse(&parser, &event) && event.type != YAML_STREAM_END_EVENT) {
        if (event.type == YAML_SCALAR_EVENT) {
            key = (char *) event.data.scalar.value;

            value = next_value(&parser);
            if (!value) {
                yaml_event_delete(&event);
                continue;
            }

            if (strcmp(key, "address") == 0) {
                config->address = value;
            } else if (strcmp(key, "num_threads") == 0) {
                config->num_threads = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "num_messages") == 0) {
                config->num_messages = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "use_json") == 0) {
                config->use_json = strcmp(value, "true") == 0;
                free(value);
            } else if (strcmp(key, "save_interval_seconds") == 0) {
                config->save_interval_seconds = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "stats_filepath") == 0) {
                config->stats_filepath = value;
            } else if (strcmp(key, "use_msg_per_time") == 0) {
                config->use_msg_per_time = strcmp(value, "true") == 0;
            } else if (strcmp(key, "msg_per_minute") == 0) {
                config->msg_per_minute = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "message_size") == 0) {
                config->message_size = convert_to_int(value);
                free(value);
            }
                // CLIENT
            else if (strcmp(key, "sleep_starting_time") == 0) {
                config->client_action->sleep_starting_time = convert_to_int(value);
                free(value);
            }

                // SERVER
            else if (strcmp(key, "sleep_starting_time") == 0) {
                config->server_action->sleep_starting_time = convert_to_int(value);
                free(value);

                // -----------------------------------------------------------------------------------------------------
            } else {
                fprintf(stderr, "Unknown key: %s\n", key);
                free(value);
            }
        }
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}

/**
 * Release the memory allocated by the configuration.
 * @param config The configuration struct.
 */
void release_config(Config *config) {
    free(config->address);
    free(config->stats_filepath);
}

// Return a string representation of the configuration.
char *get_configuration(Config config) {
    char *configuration = malloc(1024);
    if (!configuration) {
        fprintf(stderr, "Failed to allocate memory.\n");
        return NULL;
    } else {
        snprintf(configuration, 1024,
                 "\n------------------------------------------------\nConfiguration:\n"
                 "Address: %s\n"
                 "Number of threads: %d\n"
                 "Number of messages (x thread): %d (size %d Bytes)\n"
                 "Use messages per time: %s (%d msg/min)\n"
                 "Use JSON: %s\n"
                 "Save interval: %d s\n"
                 "Stats filepath: %s\n"
                 "Client/Server sleep starting time: %d/%d ms\n------------------------------------------------\n\n",
                 config.address,
                 config.num_threads,
                 config.num_messages,
                 config.message_size,
                 config.use_msg_per_time ? "yes" : "no", config.msg_per_minute,
                 config.use_json ? "yes" : "no",
                 config.save_interval_seconds,
                 config.stats_filepath,
                 config.client_action->sleep_starting_time, config.server_action->sleep_starting_time
        );
    }
    return configuration;
}


//int main() {
//    Config config;
//    if (read_config("config.yaml", &config) != 0) {
//        fprintf(stderr, "Failed to read config.yaml\n");
//        return 1;
//    }
//
//    printf("Address: %s\n", config.address);
//    printf("Number of threads: %d\n", config.num_threads);
//    printf("Number of messages: %d\n", config.num_messages);
//    release_config(&config);
//    return 0;
//}
