#include "qos.h"
#include "logger.h"  // Include the logger header for logging functionalities
#include <zmq.h>
#include <string.h>
#include <unistd.h>  // Provides access to the POSIX operating system API

#define HEARTBEAT_INTERVAL 10  // Heartbeat interval in seconds

// Last connection time
time_t last_connection_time = 0;

// Helper function to send heartbeats
void send_heartbeat(void *socket) {

    if (last_connection_time == 0) {
        last_connection_time = time(NULL);
    } else if (time(NULL) - last_connection_time > HEARTBEAT_INTERVAL) {
        // Define the heartbeat message
        char heartbeat_message[] = "HEARTBEAT";

        // Send the heartbeat message over the provided socket
        if (zmq_send(socket, heartbeat_message, strlen(heartbeat_message), 0) < 0) {
            // Log an error if unable to send
            logger(LOG_LEVEL_ERROR, "Failed to send heartbeat: %s", zmq_strerror(errno));
        } else {
            // Log that the heartbeat was sent successfully
            logger(LOG_LEVEL_INFO, "Heartbeat sent");
        }
        last_connection_time = time(NULL);
    } else {
        // Log that the heartbeat was not sent
//        logger(LOG_LEVEL_DEBUG, "Heartbeat not sent");
    }
}

// Helper function to handle reconnection
void try_reconnect(void *context, void **socket, const char *connection_string, int socket_type) {
    // Check the status of the socket
    int socket_status;
    size_t socket_status_size = sizeof(socket_status);
    zmq_getsockopt(*socket, ZMQ_EVENT_DISCONNECTED, &socket_status, &socket_status_size);

    // If the socket is disconnected, attempt to reconnect
    if (socket_status == ZMQ_EVENT_DISCONNECTED) {
        logger(LOG_LEVEL_WARN, "Connection lost, attempting to reconnect...");

        // Close the current socket
        zmq_close(*socket);

        // Create a new socket
        *socket = zmq_socket(context, socket_type);

        // Attempt to reconnect the socket
        if (zmq_connect(*socket, connection_string) == 0) {
            logger(LOG_LEVEL_INFO, "Reconnected successfully");
        } else {
            logger(LOG_LEVEL_ERROR, "Failed to reconnect: %s", zmq_strerror(errno));
        }
    } else {
        // Connection is OK
//        logger(LOG_LEVEL_DEBUG, "Connection is active, no action needed");
    }
}
