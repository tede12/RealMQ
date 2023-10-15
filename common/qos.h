//  =====================================================================
//  qos.h
//
//  Quality of Service functions
//  =====================================================================

#ifndef QOS_H
#define QOS_H

#include <zmq.h>
#include <stdbool.h>
#include <time.h>

void send_heartbeat(void *socket);

void try_reconnect(void *context, void **socket, const char *connection_string, int socket_type);


#endif // QOS_H
