#include "qos/accrual_detector.h"
#include "utils/time_utils.h"
#include "core/config.h"
#include "core/logger.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>

// Global flag to indicate shutdown
volatile bool stop_requested = false;

// Global logger
Logger client_logger;

// Function for thread
void *wait_for_q(void *arg) {
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term); // get current terminal attributes
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO); // set non-canonical mode and disable echo
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term); // apply new settings

    char ch;
    while (true) {
        read(STDIN_FILENO, &ch, 1); // read one character
        if (ch == 'q' || ch == 'Q') {
            stop_requested = true;
            usleep(200);
            printf("\n\n**** Stop requested ****\n\n");
            break;
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term); // restore old settings
    return NULL;
}


void test_accrual_detector(void) {


    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 1,
            .show_logger_name = 1,
            .show_thread_id = 1,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO
    };

    Logger_init("realmq_sever", &logger_config, &client_logger);

    if (read_config("../config.yaml") != 0) {
        logger(LOG_LEVEL_ERROR, "Failed to read config.yaml");
        return;
    }

    // For all messages this is called
    send_heartbeat(NULL, NULL, true);

    bool update = false;


    // -----------------------------------------------------------------------------------------------------------------
    for (size_t i = 0; i < 1000000 && !stop_requested; i++) {

        if (update) {
            // (send cumulative ack) and from the response back it knows how many messages were missed

            // Simulate missed messages (0 to 9)
            int missed_count = i % 4 == 0 ? rand() % 10 : 0;
            update_phi_detector(missed_count);
            update = false;

            logger(LOG_LEVEL_INFO, "Missed count: %d", missed_count);
        }

        // For all messages this is called
        if (send_heartbeat(NULL, NULL, false)) {
            update = true;
        }

        rand_sleep(0, 50);
    }
    // -----------------------------------------------------------------------------------------------------------------
}

int main(void) {

    pthread_t thread_id;
    pthread_create(&thread_id, NULL, wait_for_q, NULL);

    // Run the test
    test_accrual_detector();

    // Wait for the thread to finish
    pthread_join(thread_id, NULL);

    // release resources
    release_logger();
    release_config();

    return 0;
}
