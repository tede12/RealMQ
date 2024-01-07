#include "state.h"
#include "utils/memory_leak_detector.h"

/**
 * Initialize a new state
 * @param history The history to use for the state
 * @param timestamp The timestamp to use for the state
 * @return The new state
 */
state_t *state_init(heartbeat_history_t *history, long long timestamp) {
    state_t *state = (state_t *) malloc(sizeof(state_t));
    heartbeat_history_t *new_history = NULL;

    if (state == NULL) {
        return NULL;
    }

    if (history == NULL) {
        new_history = new_heartbeat_history(1, NULL, 0, 0.f, 0.f);
    } else {
        new_history = malloc(sizeof(heartbeat_history_t));
        if (new_history == NULL) {
            perror("Failed to allocate heartbeat_history_t");
            delete_state(state, true);
            return NULL;
        }
        update_history(new_history, history);
    }

    state->history = new_history;

    state->timestamp = timestamp;

    return state;
}

/**
 * Compare two states
 * @param first The first state
 * @param second The second state
 * @return True if the states are equal, false otherwise
 */
bool compare_states(state_t *first, state_t *second) {
    if (first == NULL || second == NULL) return false;
    if (first->timestamp == 0 && second->timestamp == 0) return true;

    return (first->timestamp == second->timestamp) &&
           (compare_history(first->history, second->history));
}

/**
 * Delete a state
 * @param state The state to delete
 * @param delete_history Whether to delete the history as well
 */
void delete_state(state_t *state, bool delete_history) {
    if (state == NULL) {
        return;
    }

    if (delete_history) {
        delete_heartbeat_history(state->history);
    } else {
        state->history = NULL;
    }

    free(state);
    state = NULL;
}

/**
 * Update a state
 * @param first The state to update
 * @param second The state to update with
 */
void update_state(state_t *first, state_t *second) {
    if (first == NULL || second == NULL) return;

    first->timestamp = second->timestamp;
    update_history(first->history, second->history);
}

