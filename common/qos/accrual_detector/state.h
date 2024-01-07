//  =====================================================================
//  state.h
//
//  State of the accrual failure detector
//  =====================================================================


#ifndef STATE_H
#define STATE_H

#include "heartbeat_history.h"
#include <time.h>
#include <stdbool.h>

typedef struct {
    heartbeat_history_t *history;
    long long timestamp;
} state_t;

state_t *state_init(heartbeat_history_t *history, long long timestamp);

bool compare_states(state_t *first, state_t *second);

void delete_state(state_t *state, bool delete_history);

void update_state(state_t *first, state_t *second);

#endif //STATE_H
