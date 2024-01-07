//  =====================================================================
//  heartbeat_history.h
//
//  Heartbeat history implementation.
//  =====================================================================


#ifndef HEARTBEAT_HISTORY_H
#define HEARTBEAT_HISTORY_H

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

typedef struct {
    size_t max_sample_size;
    float *intervals;
    size_t interval_count;
    float interval_sum;
    float squared_interval_sum;
} heartbeat_history_t;

heartbeat_history_t *
new_heartbeat_history(size_t max_sample_size, float *intervals, size_t interval_count, float interval_sum,
                      float squared_interval_sum);

void delete_heartbeat_history(heartbeat_history_t *history);

float mean(const heartbeat_history_t *history);

float variance(const heartbeat_history_t *history);

float std_dev(const heartbeat_history_t *history);

void drop_oldest_interval(heartbeat_history_t *history);

void add_interval(heartbeat_history_t *history, float interval);


float get_scaling_factor(int missed_count);

void adjust_intervals(heartbeat_history_t *history, int missed_count);

bool compare_history(heartbeat_history_t *history1, heartbeat_history_t *history2);

void update_history(heartbeat_history_t *first, heartbeat_history_t *second);


#endif //HEARTBEAT_HISTORY_H
