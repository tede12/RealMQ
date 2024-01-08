//  =====================================================================
//  phi_accrual_failure_detector.h
//
//  Phi Accrual Failure Detector implementation.
//  =====================================================================


#ifndef PHI_ACCRUAL_FAILURE_DETECTOR_H
#define PHI_ACCRUAL_FAILURE_DETECTOR_H

#include <stdlib.h>
#include <stdbool.h>
#include <utime.h>
#include "state.h"
#include "heartbeat_history.h"
#include <pthread.h>

typedef struct {
    float threshold;
    size_t max_sample_size;
    float min_std_deviation_ms;
    float acceptable_heartbeat_pause_ms;
    float first_heartbeat_estimate_ms;
    state_t *state;
} phi_accrual_detector;

// Mutex for the compare_and_set function
extern pthread_mutex_t compare_mutex;
extern phi_accrual_detector *g_detector;

void init_phi_accrual_detector(phi_accrual_detector *detector);

phi_accrual_detector *new_phi_accrual_detector(
        float threshold,
        size_t max_sample_size,
        float min_std_deviation_ms,
        float acceptable_heartbeat_pause_ms,
        float first_heartbeat_estimate_ms,
        state_t *state);

void delete_phi_accrual_detector(phi_accrual_detector *detector);

bool is_available(phi_accrual_detector *detector, long long timestamp);

float get_phi(phi_accrual_detector *detector, long long timestamp);

void heartbeat(phi_accrual_detector *detector);

heartbeat_history_t *first_heartbeat(phi_accrual_detector *detector);

float ensure_valid_std_deviation(phi_accrual_detector *detector, float std_deviation);

bool compare_and_set(state_t *current, state_t *expect, state_t *update);

#endif //PHI_ACCRUAL_FAILURE_DETECTOR_H
