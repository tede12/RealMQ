#include "unity.h"
#include <stdio.h>
#include "core/config.h"


void setUp(void) {
    // Create file config for testing in /tmp
    FILE *fp = fopen("/tmp/config.yaml", "w");
    fprintf(fp, "# config.yaml\n\n# General settings\ngeneral:\n");
    // Using double space indentation
    fprintf(fp, "  stats_folder_path: tmp\n");
    fprintf(fp, "  protocol: tcp\n");
    fprintf(fp, "  use_json: 0\n");
    fclose(fp);


    // set stuff up here
    if (read_config("/tmp/config.yaml") != 0) {
        TEST_FAIL_MESSAGE("Failed to read config.yaml");
        TEST_ASSERT_EQUAL_INT(0, 1); // force the test to fail
    }
}

void tearDown(void) {
    release_config();
    // Remove file
    remove("/tmp/config.yaml");
}


void test_read_configuration(void) {
    TEST_ASSERT_EQUAL_STRING("tmp", config.stats_folder_path);
    TEST_ASSERT_EQUAL_STRING("tcp", config.protocol);
    TEST_ASSERT_EQUAL_INT(0, config.use_json);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_read_configuration);
    return UNITY_END();
}
