#include "config.h"
#include "logger.h"
#include <signal.h> // for raise of SIGINT
#include <zmq.h>

Config config;  // The global definition of the configuration
char *g_ip_address = NULL;
pthread_mutex_t g_ip_address_mutex = PTHREAD_MUTEX_INITIALIZER;


/**
 * Get the address of the responder or receiver. Schema: <protocol>://<ip>:<port>
 * @param address_type
 * @return
 */
char *get_address(AddressType address_type) {
    pthread_mutex_lock(&g_ip_address_mutex);
    if (g_ip_address != NULL) {
        free(g_ip_address);
        g_ip_address = NULL;
    }  // prevent memory leak if the function is called multiple times
    g_ip_address = (char *) calloc(64, sizeof(char));

    // Ensure that the memory was allocated successfully
    if (g_ip_address == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory for full address.");
        return NULL;
    }

    const char *address;
    switch (address_type) {
        case RESPONDER_ADDRESS:
            address = config.responder_address;
            break;
        case MAIN_ADDRESS:
            address = config.main_address;
            break;
        default:
            logger(LOG_LEVEL_ERROR, "Invalid address type.");
            free(g_ip_address);
            g_ip_address = NULL;
            return NULL;
    }

    int written = snprintf(g_ip_address, 64, "%s://%s", config.protocol, address);
    if (written < 0 || written >= 64) {
        logger(LOG_LEVEL_ERROR, "Error or insufficient space while composing the full address.");
        free(g_ip_address);
        g_ip_address = NULL;
        return NULL;
    }

    pthread_mutex_unlock(&g_ip_address_mutex);
    return g_ip_address;
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

ProtocolType get_protocol_type(void) {
    if (strcmp(config.protocol, "tcp") == 0) {
        return TCP;
    } else if (strcmp(config.protocol, "udp") == 0) {
        return UDP;
    } else {
        logger(LOG_LEVEL_ERROR, "Not handled protocol: %s", config.protocol);
        raise(SIGINT);
        return -1;
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

#ifdef ZMQ_BUILD_DRAFT_API
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
int convert_string_to_int(const char *value) {
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
 * Assign the key-value pair to the configuration.
 * @param latest_section
 * @param key
 * @param value
 */
void assign_key_value(char *latest_section, char *key, char *value) {
    // Remember to free Integer values
    if (strcmp(latest_section, "general") == 0) {
        if (strcmp(key, "main_address") == 0) {
            config.main_address = strdup(value);
            return;
        } else if (strcmp(key, "responder_address") == 0) {
            config.responder_address = strdup(value);
            return;
        } else if (strcmp(key, "num_threads") == 0) {
            config.num_threads = convert_string_to_int(value);
            return;
        } else if (strcmp(key, "num_messages") == 0) {
            config.num_messages = convert_string_to_int(value);
            return;
        } else if (strcmp(key, "use_json") == 0) {
            config.use_json = strcmp(value, "true") == 0;
            return;
        } else if (strcmp(key, "save_interval_seconds") == 0) {
            config.save_interval_seconds = convert_string_to_int(value);
            return;
        } else if (strcmp(key, "stats_folder_path") == 0) {
            config.stats_folder_path = strdup(value);
            return;
        } else if (strcmp(key, "use_msg_per_minute") == 0) {
            config.use_msg_per_minute = strcmp(value, "true") == 0;
            return;
        } else if (strcmp(key, "msg_per_minute") == 0) {
            config.msg_per_minute = convert_string_to_int(value);
            return;
        } else if (strcmp(key, "message_size") == 0) {
            config.message_size = convert_string_to_int(value);
            return;
        } else if (strcmp(key, "protocol") == 0) {
            config.protocol = strdup(value);
            return;
        } else if (strcmp(key, "signal_msg_timeout") == 0) {
            config.signal_msg_timeout = convert_string_to_int(value);
            return;
        }
    }
    if (strcmp(latest_section, "client") == 0) {
        if (strcmp(key, "sleep_starting_time") == 0) {
            config.client_action->sleep_starting_time = convert_string_to_int(value);
            return;
        }
    }
    if (strcmp(latest_section, "server") == 0) {
        if (strcmp(key, "sleep_starting_time") == 0) {
            config.server_action->sleep_starting_time = convert_string_to_int(value);
            return;
        }
    }

    logger(LOG_LEVEL_ERROR, "Unknown key: %s", key);
}

/**
 * Read the configuration from a YAML file.
 * @param filename The name of the YAML file.
 * @param config The configuration struct to populate.
 * @return 0 if the configuration was read successfully, -1 otherwise.
 */
int read_config(const char *filename) {
    // Open the file
    FILE *fh = fopen(filename, "r");
    if (!fh) {
        logger(LOG_LEVEL_ERROR, "Failed to open %s", filename);
        return -1;
    }

    // Initialize the parser
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

    char *latest_section = NULL;

    yaml_event_t event;
    char *key = NULL, *value = NULL;
    int loop_count = 0;

    while (yaml_parser_parse(&parser, &event) && event.type != YAML_STREAM_END_EVENT) {
        loop_count++;
        if (loop_count > 20) {
            logger(LOG_LEVEL_ERROR, "Loop count exceeded. File is probably malformed.");
            raise(SIGINT);
        }
        if (event.type == YAML_SCALAR_EVENT) {
            loop_count = 0; // Reset the loop count

            key = (char *) event.data.scalar.value;
            value = next_value(&parser);

            // If the value is NULL, then the key is a fixed value (e.g. "general", "client", "server" sections)
            if (key && value == NULL) {
                if (latest_section) free(latest_section);
                latest_section = malloc(strlen(key) + 1);
                strcpy(latest_section, key);
            }

            if (!value) {
                yaml_event_delete(&event);
                continue;
            }

            // Assign the key-value pair to the configuration
            assign_key_value(latest_section, key, value);
            free(key);
            free(value);
            key = NULL;
            value = NULL;
        }
    }

    if (latest_section) free(latest_section);
    if (key) free(key);
    if (value) free(value);

    yaml_event_delete(&event);
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}


/**
 * Release the memory allocated by the configuration.
 * @param config The configuration struct.
 */
void release_config() {
    printf("\n\n");
    logger(LOG_LEVEL_DEBUG, "Releasing configuration.");
    // Only need to free the Strings, the rest is allocated statically

    // Array of pointers-to-pointers to hold addresses of fields to free.
    void **fields_to_free[] = {
            (void **) &config.main_address,
            (void **) &config.responder_address,
            (void **) &config.stats_folder_path,
            (void **) &config.protocol,
            (void **) &config.client_action->name,
            (void **) &config.server_action->name,
            (void **) &config.client_action,
            (void **) &config.server_action,
            NULL  // Sentinel value to mark the end of the array.
    };

    // Iterate over the array and free each field and set to NULL.
    for (int i = 0; fields_to_free[i] != NULL; i++) {
        if (*fields_to_free[i] == NULL) continue;

        free(*fields_to_free[i]);
        *fields_to_free[i] = NULL;
    }

    // Free the full address
    if (g_ip_address) {
        free(g_ip_address);
        g_ip_address = NULL;
    }
}

// Return a string representation of the configuration.
void print_configuration() {
    char *configuration = malloc(1024);
    if (configuration == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory.");
        return;
    }

    char qos_flag[4];  // "yes" or "no" plus null terminator
#ifdef REALMQ_VERSION
    snprintf(qos_flag, sizeof(qos_flag), "yes");
#else
    snprintf(qos_flag, sizeof(qos_flag), "no");
#endif

    char *main_address = malloc(32);
    char *responder_address = malloc(32);

    if (main_address == NULL || responder_address == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to allocate memory.");
        return;
    }

    snprintf(main_address, 65, "%s", get_address(MAIN_ADDRESS));
    snprintf(responder_address, 64, "%s", get_address(RESPONDER_ADDRESS));

    snprintf(configuration, 1024,
             "\n------------------------------------------------\nConfiguration:\n"
             "Address Main/Responder: %s, %s\n"
             "Number of threads: %d\n"
             "Number of messages (x thread): %d (size %d Bytes)\n"
             "Total messages: %d\n"
             "Use messages per minute: %s (%d msg/min)\n"
             "Use JSON: %s\n"
             "Save interval: %d s\n"
             "Stats filepath: %s\n"
             "Client/Server sleep starting time: %d/%d ms\n"
             "------------------------------------------------\n"
             "PROTOCOL: %s\n"
             "QOS: %s\n"
             "------------------------------------------------\n\n",
             main_address, responder_address,
             config.num_threads,
             config.num_messages,
             config.message_size,
             config.num_threads * config.num_messages,
             config.use_msg_per_minute ? "yes" : "no", config.msg_per_minute,
             config.use_json ? "yes" : "no",
             config.save_interval_seconds,
             config.stats_folder_path,
             config.client_action->sleep_starting_time, config.server_action->sleep_starting_time,
             config.protocol,
             qos_flag
    );

    logger(LOG_LEVEL_DEBUG, configuration);
    free(main_address);
    free(responder_address);
    free(configuration);
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
