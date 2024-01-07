#include "core/logger.h"
#include "qos/accrual_detector/heartbeat_history.h"
#include "qos/accrual_detector/state.h"
#include "qos/accrual_detector/phi_accrual_failure_detector.h"
#include "utils/time_utils.h"
#include "utils/memory_leak_detector.h"

#define NUM_OF_MESSAGES 100

/*
 * This simulator is used to test the phi accrual failure detector.
 * With the use of the logging, It is capable of sending information about the state of the detector to the
 * `live_graphs.py` tool. This tool can be used to plot the phi values over time.
 * Missed messages can be simulated by setting the `miss_sequence` array, which is used to simulate missed messages.
 */

// pop from back of list and return zero if index is out of bounds
int pop_from_back(int *list, int size, int index) {
    if (index >= size) {
        return 0;
    }
    return list[size - index - 1];

}

void test_accrual_detector() {
    phi_accrual_detector *detector = new_phi_accrual_detector(
            6,
            1000,
            10,
            0,
            1000,
            NULL
    );

    logger(LOG_LEVEL_INFO, "CLEAN DATA");

    heartbeat(detector);

    bool update = false;

    // Set the sequence of missed messages
    int miss_sequence[] = {1, 3, 5, 0, 0, 0, 0, 0, 0};
    int count = 0;

    for (size_t i = 0; i < NUM_OF_MESSAGES; i++) {
        if (update) {
            int missed_count = pop_from_back(miss_sequence, 9, count);
            count++;

            update = false;
            logger(LOG_LEVEL_INFO, "Missed count: %d", missed_count);

            // Update failure detector based on missed_count
            state_t *current_state = detector->state;
            adjust_intervals(current_state->history, missed_count);
        }

        // For all messages this is called (if phi > threshold, we need to send an ack)
        if (!is_available(detector, 0)) {
            update = true;
            heartbeat(detector);
            logger(LOG_LEVEL_INFO, "Send ack");
            logger(LOG_LEVEL_INFO, "Heartbeat sent");
        }

        float phi = get_phi(detector, 0);  // for all messages this is called
        logger(LOG_LEVEL_INFO2,
               "Phi: %8.4lf, Plater: %8.4lf, Mean: %8.4lf, Variance: %8.4lf", phi, 0, 0, 0);


        rand_sleep(0, 1000);
    }

    delete_phi_accrual_detector(detector);

}


int main() {
    Logger main_logger;
    // Load the configuration
    logConfig logger_config = {
            .show_timestamp = 0,
            .show_logger_name = 0,
            .show_thread_id = 0,
            .log_to_console = 1,
            .log_level = LOG_LEVEL_INFO,
            .log_path = "../logger.log"

    };

    // Initialize the logger
    Logger_init("accrual_detector", &logger_config, &main_logger);

    test_accrual_detector();

    // Release resources
    release_logger();

    // Check for memory leaks
    check_for_leaks();

    return 0;
}
