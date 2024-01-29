#include "accrual_detector.h"
#include "core/logger.h"
#include <zmq.h>
#include "core/config.h"
#include "core/zhelpers.h"
#include "qos/accrual_detector/phi_accrual_failure_detector.h"

// =====================================================================================================================
/* How it works (in a nutshell):
 * This function is called periodically by the client to send a heartbeat message to the server.
 * The timeout is calculated by the server based on the last time it received a heartbeat message.
 * It's calculated with an algorithm called "Phi Accrual Failure Detector". The server will consider the client
 * disconnected if it doesn't receive a heartbeat message for a certain amount of time. This is important for
 * cleaning up the resources used by the client (in this case for removing the message queue).
 */

bool log_heartbeat = false;  // Set to true to log the heartbeat messages

/**
 * Function that sends a heartbeat message to the server and updates the
 * Phi Accrual Failure Detector parameters.
 * @param socket The socket to use for sending the message
 */
bool send_heartbeat(void *socket, const char *group, bool force_send) {
    char heartbeat_message[] = "HB";

    if (force_send) {
        // Send the first heartbeat
        if (g_detector->state->timestamp != 0) {
            // Case for sending a heartbeat message before disconnecting
            // (so that the server can detect the latest messages sent)
            if (socket != NULL) zmq_send_group(socket, group, heartbeat_message, 0);
            return true;
        }
        heartbeat(g_detector);
        return true;
    }

    bool is_sent = false;

    // Send a heartbeat before starting to send messages
    if (!is_available(g_detector, 0)) {
        heartbeat(g_detector);

        // Send the heartbeat message
        if (socket != NULL) {
            if (zmq_send_group(socket, group, heartbeat_message, 0) < 0) {
                logger(LOG_LEVEL_ERROR, "Failed to send heartbeat: %s", zmq_strerror(errno));
            } else {
                if (log_heartbeat) logger(LOG_LEVEL_INFO2, "Heartbeat sent");
                is_sent = true;
            }
        }
    }

    // Update the parameters of the Phi Accrual Failure Detector
    float phi = get_phi(g_detector, 0); // for all messages this is called
    if (log_heartbeat)
        logger(LOG_LEVEL_INFO2, "Phi: %8.4lf, Plater: %8.4lf, Mean: %8.4lf, Variance: %8.4lf", phi, 0, 0, 0);
    return is_sent;
}


/**
 * Function to handle reconnection of the socket in case of disconnection.
 * This function is used by the client only for TCP connections.
 * @param context
 * @param socket
 * @param connection_string
 * @param socket_type
 */
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
            // Set a timeout for receive operations
            zmq_setsockopt(socket, ZMQ_RCVTIMEO, &config.signal_msg_timeout, sizeof(config.signal_msg_timeout));

        } else {
            logger(LOG_LEVEL_ERROR, "Failed to reconnect: %s", zmq_strerror(errno));
        }
    } else {
        // Connection is OK
//        logger(LOG_LEVEL_DEBUG, "Connection is active, no action needed");
    }
}
