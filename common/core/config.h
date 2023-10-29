//  =====================================================================
//  config.h
//
//  Configuration functions
//  =====================================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdbool.h>
#include <yaml.h>
#include <errno.h>
#include <stdlib.h>  // for strtol and free
#include <string.h>
#include <unistd.h> // for sleep function

typedef struct ActionType {
    char *name;
    int sleep_starting_time;
} ActionType;

/**
 * The configuration struct.
 */
typedef struct Config {
    char *main_address;
    char *responder_address;
    char *protocol;
    int num_threads;
    int num_messages;
    bool use_json;
    bool use_msg_per_minute;
    int msg_per_minute;
    int save_interval_seconds;
    int message_size;
    char *stats_folder_path;
    int signal_msg_timeout;
    ActionType *client_action;
    ActionType *server_action;
} Config;

typedef enum {
    MAIN_ADDRESS,
    RESPONDER
} AddressType;

typedef enum {
    MAIN_GROUP,
    RESPONDER_GROUP
} GroupType;

typedef enum {
    SERVER,
    CLIENT
} RoleType;


extern Config config;
extern char *g_ip_address;

char *get_address(AddressType address_type);

const char *get_group(GroupType group_type);

int get_zmq_type(RoleType roleType);

char *next_value(yaml_parser_t *parser);

int convert_string_to_int(const char *value);

void assign_key_value(char *latest_section, char *key, char *value);

int read_config(const char *filename);

void release_config();

void print_configuration();

#endif //CONFIG_H
















