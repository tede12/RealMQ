#include "unity.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h> // For rmdir
#include "utils/fs_utils.h"
#include "utils/time_utils.h"
#include "core/config.h"


void setUp(void) {
    // set stuff up here
    if (read_config("../config.yaml") != 0) {
        TEST_FAIL_MESSAGE("Failed to read config.yaml");
        TEST_ASSERT_EQUAL_INT(0, 1); // force the test to fail
    }

    // This is run before EACH test
    date_time = strdup("01_01_2021_00_00_00");

    // Example setup with stats_folder_path = "tmp" and protocol = "tcp"
    if (config.stats_folder_path != NULL) {
        free(config.stats_folder_path);  // Free existing memory to avoid memory leak
    }
    config.stats_folder_path = strdup("tmp");  // Duplicate the string literal

    if (config.protocol != NULL) {
        free(config.protocol);  // Free existing memory to avoid memory leak
    }
    config.protocol = strdup("tcp");  // Duplicate the string literal
    config.use_json = 0;
}

void tearDown(void) {
    // This is run after EACH test
    // Clean up any allocated memory, temporary files, or directories here
    if (date_time) {
        free(date_time);
        date_time = NULL;
    }
    rmdir(config.stats_folder_path); // Removing temporary directory
    release_config(); // Release the config
}


void test_file_exists(void) {
    // Create a temporary file for testing
    FILE *fp = fopen("/tmp/test_file", "w");
    fclose(fp);

    check_file_exists("/tmp/test_file");
    TEST_ASSERT_EQUAL_INT(1, check_file_exists("/tmp/test_file"));

    // Cleanup
    remove("/tmp/test_file");
}

void test_create_stats_path_valid(void) {
    char *path = create_stats_path();
    TEST_ASSERT_NOT_NULL(path);
    // We're checking the path format based on the mocked 'get_current_date_time' function
    TEST_ASSERT_EQUAL_STRING("tmp/01_01_2021_00_00_00_tcp_0_result.csv", path);
    free(path); // Cleanup
}

void test_save_stats_to_file_null_json(void) {
    json_object *json = NULL;
    save_stats_to_file(&json);
    // Assuming the function should not create a file when JSON data is NULL
    struct stat buffer;
    int exist = stat("tmp/01_01_2021_00_00_00_tcp_0_result.csv", &buffer);
    TEST_ASSERT_EQUAL(-1, exist); // File should not exist
    TEST_ASSERT_EQUAL(ENOENT, errno); // Confirming that it's a "Does not exist" error
}

void test_save_stats_to_file_creates_file(void) {
    json_object *json = json_object_new_array();
    json_object_array_add(json, json_object_new_string("test_message")); // Adding a test message

    // Creating a temp directory for test
    const char *dirname = "tmp";
    mkdir(dirname, 0700);

    save_stats_to_file(&json);

    // Check if the file has been created
    struct stat buffer;
    int exist = stat("tmp/01_01_2021_00_00_00_tcp_0_result.csv", &buffer);
    TEST_ASSERT_EQUAL(0, exist); // File should exist

    // Optionally: read the file to check if the contents are as expected

    // Cleanup
    json_object_put(json); // free json object
    remove("tmp/01_01_2021_00_00_00_tcp_0_result.csv"); // delete the created file
}

// More tests...

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_create_stats_path_valid);
    RUN_TEST(test_save_stats_to_file_null_json);
    RUN_TEST(test_save_stats_to_file_creates_file);
    RUN_TEST(test_file_exists);
    // More RUN_TEST() calls...
    return UNITY_END();
}
