#include "config.h"
#include "logger.h"
#include <zmq.h>

Config config;  // The global definition of the configuration

/**
 * Get the address of the responder or receiver. Example: tcp://ip:port
 * @param address_type
 * @return
 */
char *get_address(AddressType address_type) {
    // protocol://ip:port
    char *full_address = (char *) malloc(64 * sizeof(char));

    // Ensure that the memory was allocated successfully
    if (full_address == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory for full address.");
        return NULL;
    }

    const char *address;
    switch (address_type) {
        case RESPONDER:
            address = config.responder_address; // Use the correct address here
            break;
        case MAIN_ADDRESS:
            address = config.main_address;
            break;
        default:
            logger(LOG_LEVEL_ERROR, "Invalid address type.");
            free(full_address);
            return NULL;
    }

    int written = snprintf(full_address, 64, "%s://%s", config.protocol, address);
    if (written < 0 || written >= 64) {
        logger(LOG_LEVEL_ERROR, "Error or insufficient space while composing the full address.");
        free(full_address);
        return NULL;
    }

    return full_address;
}

/**
 * Get the group name based on the group type.
 * @param group_type The group type.
 * @return The group name.
 */
const char *get_group(GroupType group_type) {
    switch (group_type) {
        case MAIN_GROUP:
            return "GRP";
        case RESPONDER_GROUP:
            return "REP";
        default:
            logger(LOG_LEVEL_ERROR, "Invalid group type.");
            return NULL;
    }
}

/**
 * Get the ZMQ socket type based on the protocol.
 * @return The ZMQ socket type.
 */
int get_zmq_type(RoleType roleType) {
    if (strcmp(config.protocol, "tcp") == 0) {
        if (roleType == SERVER) {
            return ZMQ_SUB;
        } else if (roleType == CLIENT) {
            return ZMQ_PUB;
        }

    }

#define DZMQ_BUILD_DRAFT_API    // todo need to be fixed
#ifdef DZMQ_BUILD_DRAFT_API
        // This code is only compiled if the ZMQ BUILD DRAFT is enabled otherwise it will raise an error
    else if (strcmp(config.protocol, "udp") == 0) {
        if (roleType == SERVER) {
            return ZMQ_DISH;
        } else if (roleType == CLIENT) {
            return ZMQ_RADIO;
        }
    }
#endif

    else {
        logger(LOG_LEVEL_ERROR, "Invalid protocol: %s", config.protocol);
        raise(SIGINT);
        return -1;
    }

    logger(LOG_LEVEL_ERROR, "Invalid address type.");
    raise(SIGINT);
    return -1;
}


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
        logger(LOG_LEVEL_ERROR, "Conversion error or out of range: %s", value);
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
int read_config(const char *filename) {
    FILE *fh = fopen(filename, "r");
    if (!fh) {
        logger(LOG_LEVEL_ERROR, "Failed to open %s", filename);
        return -1;
    }

    yaml_parser_t parser;
    if (!yaml_parser_initialize(&parser)) {
        logger(LOG_LEVEL_ERROR, "Failed to initialize the YAML parser.");
        return -1;
    }
    yaml_parser_set_input_file(&parser, fh);

    // Allocate and initialize
    config.client_action = malloc(sizeof(ActionType));
    config.server_action = malloc(sizeof(ActionType));
    if (!config.client_action || !config.server_action) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory.");
        return -1;
    }

    // Default values
    config.client_action->name = strdup("CLIENT");
    config.server_action->name = strdup("SERVER");

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
            // Remember to free Integer values
            if (strcmp(key, "main_address") == 0) {
                config.main_address = value;
            } else if (strcmp(key, "responder_address") == 0) {
                config.responder_address = value;
            } else if (strcmp(key, "num_threads") == 0) {
                config.num_threads = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "num_messages") == 0) {
                config.num_messages = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "use_json") == 0) {
                config.use_json = strcmp(value, "true") == 0;
                free(value);
            } else if (strcmp(key, "save_interval_seconds") == 0) {
                config.save_interval_seconds = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "stats_folder_path") == 0) {
                config.stats_folder_path = value;
            } else if (strcmp(key, "use_msg_per_minute") == 0) {
                config.use_msg_per_minute = strcmp(value, "true") == 0;
            } else if (strcmp(key, "msg_per_minute") == 0) {
                config.msg_per_minute = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "message_size") == 0) {
                config.message_size = convert_to_int(value);
                free(value);
            } else if (strcmp(key, "protocol") == 0) {
                config.protocol = value;
            } else if (strcmp(key, "signal_msg_timeout") == 0) {
                config.signal_msg_timeout = convert_to_int(value);
                free(value);
            }
                // CLIENT
            else if (strcmp(key, "sleep_starting_time") == 0) {
                config.client_action->sleep_starting_time = convert_to_int(value);
                free(value);
            }

                // SERVER
            else if (strcmp(key, "sleep_starting_time") == 0) {
                config.server_action->sleep_starting_time = convert_to_int(value);
                free(value);

                // -----------------------------------------------------------------------------------------------------
            } else {
                logger(LOG_LEVEL_ERROR, "Unknown key: %s", key);
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
void release_config() {
    // Only need to free the Strings, the rest is allocated statically
    free(config.main_address);
    free(config.responder_address);
    free(config.stats_folder_path);
    free(config.client_action->name);
    free(config.server_action->name);
    free(config.client_action);
    free(config.server_action);
    free(config.protocol);
}

// Return a string representation of the configuration.
char *get_configuration() {
    char *configuration = malloc(1024);
    if (!configuration) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory.");
        return NULL;
    } else {

        char qos_flag[7];

#ifdef REALMQ_VERSION
        snprintf(qos_flag, 7, "yes");
#else
        snprintf(qos_flag, 7, "no");
#endif

        snprintf(configuration, 1024,
                 "\n------------------------------------------------\nConfiguration:\n"
                 "Address Main/Responder: %s, %s\n"
                 "Number of threads: %d\n"
                 "Number of messages (x thread): %d (size %d Bytes)\n"
                 "Use messages per minute: %s (%d msg/min)\n"
                 "Use JSON: %s\n"
                 "Save interval: %d s\n"
                 "Stats filepath: %s\n"
                 "Client/Server sleep starting time: %d/%d ms\n------------------------------------------------\n"
                 "PROTOCOL: %s\n"
                 "QOS: %s\n"
                 "------------------------------------------------\n\n",
                 get_address(MAIN_ADDRESS), get_address(RESPONDER),
                 config.num_threads,
                 config.num_messages,
                 config.message_size,
                 config.use_msg_per_minute ? "yes" : "no", config.msg_per_minute,
                 config.use_json ? "yes" : "no",
                 config.save_interval_seconds,
                 config.stats_folder_path,
                 config.client_action->sleep_starting_time, config.server_action->sleep_starting_time,
                 config.protocol,
                 qos_flag
        );
    }
    return configuration;
}


/**
 * Example usage of the configuration:
    Config config;
    if (read_config("config.yaml") != 0) {
        fprintf(stderr, "Failed to read config.yaml\n");
        return 1;
    }

    printf("Address: %s\n", get_address(RECEIVER));
    printf("Number of threads: %d\n", config.num_threads);
    printf("Number of messages: %d\n", config.num_messages);
    release_config();
    return 0;
*/
