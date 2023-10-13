// common/qos.h
#ifndef QOS_H
#define QOS_H

#include <zmq.h>
#include <stdbool.h>
#include <time.h>

void send_heartbeat(void *socket);
bool receive_heartbeat(void *socket);
void client_connection_manager(void *context, const char *connection_string);
void try_reconnect(void *context, void **socket, const char *connection_string);


#endif // QOS_H
