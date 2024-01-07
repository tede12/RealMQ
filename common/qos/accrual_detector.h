//  =====================================================================
//  accrual_detector.h
//
//  Quality of Service functions
//  =====================================================================

#ifndef ACCRUAL_DETECTOR
#define ACCRUAL_DETECTOR

#include <zmq.h>
#include <stdbool.h>
#include <time.h>

void update_phi_detector(size_t missed_count);

bool send_heartbeat(void *socket, const char *group, bool force_send);

void try_reconnect(void *context, void **socket, const char *connection_string, int socket_type);


#endif // ACCRUAL_DETECTOR
