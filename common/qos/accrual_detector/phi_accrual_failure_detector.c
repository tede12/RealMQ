#include "phi_accrual_failure_detector.h"
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "core/logger.h"
#include "utils/time_utils.h"
#include "utils/memory_leak_detector.h"


#define MAX(a, b) ((a > b) ? (a) : (b))

// Mutex for compare_and_set
pthread_mutex_t compare_mutex = PTHREAD_MUTEX_INITIALIZER;

phi_accrual_detector *g_detector = NULL;

/**
 * Initialize the phi_accrual_detector object for global usage with g_detector variable
 * @param detector is the phi_accrual_detector object
 */
void init_phi_accrual_detector(phi_accrual_detector *detector) {
    if (g_detector == NULL) {
        g_detector = new_phi_accrual_detector(
                detector->threshold,
                detector->max_sample_size,
                detector->min_std_deviation_ms,
                detector->acceptable_heartbeat_pause_ms,
                detector->first_heartbeat_estimate_ms,
                detector->state
        );
    }
}

/**
 * Create a new phi_accrual_detector object
 * @param threshold is the threshold value for the phi
 * @param max_sample_size is the maximum number of intervals that can be stored in the history (WINDOW_SIZE)
 * @param min_std_deviation_ms is the minimum standard deviation value
 * @param acceptable_heartbeat_pause_ms is the acceptable heartbeat pause value
 * @param first_heartbeat_estimate_ms is the first heartbeat estimate value
 * @param state is the state of the detector
 * @return a pointer to the new phi_accrual_detector object
 */
phi_accrual_detector *new_phi_accrual_detector(
        float threshold,
        size_t max_sample_size,
        float min_std_deviation_ms,
        float acceptable_heartbeat_pause_ms,
        float first_heartbeat_estimate_ms,
        state_t *state) {

    phi_accrual_detector *ret;

    ret = (phi_accrual_detector *) malloc(sizeof(phi_accrual_detector));
    if (ret == NULL) {
        return NULL;
    }

    ret->threshold = threshold;
    ret->max_sample_size = max_sample_size;
    ret->min_std_deviation_ms = min_std_deviation_ms;
    ret->acceptable_heartbeat_pause_ms = acceptable_heartbeat_pause_ms;
    ret->first_heartbeat_estimate_ms = first_heartbeat_estimate_ms;
    if (state == NULL) {
        ret->state = state_init(NULL, 0);
    } else {
        ret->state = state;
    }

    // First heartbeat
    delete_heartbeat_history(ret->state->history);
    ret->state->history = first_heartbeat(ret);
    return ret;
}

/**
 * Delete a phi_accrual_detector object
 * @param detector is the phi_accrual_detector object to delete
 */
void delete_phi_accrual_detector(phi_accrual_detector *detector) {
    if (detector == NULL) return;

    if (detector->state != NULL) {
        delete_state(detector->state, true);
    }

    free(detector);
    detector = NULL;
}


/**
 * Check if the resource is available based on the threshold and the calculated phi
 * @param detector is the phi_accrual_detector object
 * @param timestamp is the current timestamp
 * @return true if the resource is available otherwise false
 */
bool is_available(phi_accrual_detector *detector, long long timestamp) {
    if (timestamp == 0) {
        timestamp = get_current_timestamp();
    }

    float phi_value = get_phi(detector, timestamp);

//    if (phi_value > detector->threshold) {
//        logger(LOG_LEVEL_INFO, "phi_value: %8.4lf > self.threshold: %8.4lf", phi_value, detector->threshold);
//    }

    return phi_value < detector->threshold;
}


/**
 * Calculate the phi based on the current state and the Tlast.
 * @param detector is the phi_accrual_detector object
 * @param timestamp is the current timestamp
 * @return the value of the phi
 */
float get_phi(phi_accrual_detector *detector, long long timestamp) {
    float phi;
    long long last_timestamp;

    if (timestamp == 0) {
        timestamp = get_current_timestamp();
    }

    state_t *last_state = detector->state;
    if (last_state->timestamp == 0) {
        return 0.0f;
    } else {
        last_timestamp = last_state->timestamp;
    }

    long long time_diff = timestamp - last_timestamp;
    heartbeat_history_t *last_history = last_state->history;

    float mean_value = mean(last_history);
    float std_dev_value = ensure_valid_std_deviation(detector, std_dev(last_history));

    float y = ((float) time_diff - mean_value) / std_dev_value;

    float e = expf(-y * (1.5976f + 0.070566f * y * y));

    if (e == 0.0f) {
        return INFINITY;
    }

    if ((float) time_diff > mean_value) {
        phi = -log10f(e / (1.0f + e));
    } else {
        phi = -log10f(1.0f - (1.0f / (1.0f + e)));
    }

//    logger(LOG_LEVEL_INFO2,
//           "Phi: %8.4lf, Plater: %8.4lf, Mean: %8.4lf, Variance: %8.4lf", phi, 0, mean_value, std_dev_value);

    return phi;
}


/**
 * Update the state of the detector based on the current timestamp
 * @param detector is the phi_accrual_detector object
 */
void heartbeat(phi_accrual_detector *detector) {
    heartbeat_history_t *new_history = NULL;

    long long timestamp = get_current_timestamp();
    state_t *old_state = detector->state;   // copy of the old state with the old history

    if (old_state->timestamp == 0) {
        heartbeat_history_t *tmp_history = first_heartbeat(detector);

        update_history(detector->state->history, tmp_history);
        delete_heartbeat_history(tmp_history);

        new_history = detector->state->history;

    } else {
        long long latest_timestamp = old_state->timestamp;
        long long interval = timestamp - latest_timestamp;

        new_history = old_state->history;

        if (is_available(detector, timestamp)) {
            new_history += interval;
        }
    }

    state_t *new_state = state_init(new_history, timestamp);

    if (!compare_and_set(detector->state, old_state, new_state)) {
        delete_state(new_state, true);
        heartbeat(detector);
    } else {
        delete_state(new_state, true);

    }
}

/**
 * Compare two states
 * @param first is the first state
 * @param second is the second state
 * @return true if the states are equal otherwise false
 */
bool compare_and_set(state_t *current, state_t *expect, state_t *update) {
    // Lock the mutex
    bool ret = false;
    pthread_mutex_lock(&compare_mutex);
    if (compare_states(current, expect)) {
        update_state(current, update); // Swap the states (so we can free the old state)
        ret = true;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&compare_mutex);
    return ret;
}


/**
 * Initialize a new heartbeat_history_t instance using the first_heartbeat_estimate_ms
 * @param detector is the phi_accrual_detector object
 * @return a pointer to the new heartbeat_history_t object
 */
heartbeat_history_t *first_heartbeat(phi_accrual_detector *detector) {
    if (detector == NULL) {
        perror("detector is null");
        return NULL;
    }

    float mean_value = detector->first_heartbeat_estimate_ms;

    float std_dev_value = mean_value / 4.f;

    heartbeat_history_t *heartbeat_history = new_heartbeat_history(
            detector->max_sample_size, NULL, 0, 0, 0
    );

    if (heartbeat_history == NULL) {
        perror("heartbeat_history is null");
        return NULL;
    }

    add_interval(heartbeat_history, (mean_value - std_dev_value));
    add_interval(heartbeat_history, (mean_value + std_dev_value));

    return heartbeat_history;
}


/**
 * Ensure that the std_deviation is greater than the min_std_deviation_ms
 * @param detector is the phi_accrual_detector object
 * @param std_deviation is the std_deviation value to check
 * @return the maximum between a std_deviation and the minimum value configured in the constructor.
 */
float ensure_valid_std_deviation(phi_accrual_detector *detector, float std_deviation) {
    return MAX(std_deviation, detector->min_std_deviation_ms);
}

#undef MAX