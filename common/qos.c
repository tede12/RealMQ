#include "qos.h"
#include "logger.h"
#include <zmq.h>
#include <string.h>
#include <unistd.h>
#include "config.h"
#include "zhelpers.h"
#include <math.h>
#include <time.h>

// =====================================================================================================================
/* How it works (in a nutshell):
 * This function is called periodically by the client to send a heartbeat message to the server.
 * The timeout is calculated by the server based on the last time it received a heartbeat message.
 * It's calculated with an algorithm called "Phi Accrual Failure Detector". The server will consider the client
 * disconnected if it doesn't receive a heartbeat message for a certain amount of time. This is important for
 * cleaning up the resources used by the client (in this case for removing the message queue).
 */
// =====================================================================================================================


#define HEARTBEAT_INTERVAL 2 // Interval between heartbeats (in seconds)
#define PHI_THRESHOLD 5.0    // Threshold of the phi value to consider a disconnection
#define WINDOW_SIZE 10       // Size of the window for the samples, used in the rolling mean and variance

// Global variables to store the times of the last heartbeats
time_t last_heartbeat_times[WINDOW_SIZE];
int last_heartbeat_index = 0;
double mean = HEARTBEAT_INTERVAL;
double variance = 0.0;

/**
 * Function to calculate the Phi value based on the current time.
 * The formula used for phi is derived from the exponential distribution and is used to
 * quantify the suspicion level of a node failure.
 * Phi is calculated as: phi = -log10(exp(-(time since last heartbeat)/mean))
 * @param current_time The current time
 * @return The calculated phi value
 */
double calculate_phi(time_t current_time) {
    double time_diff = difftime(current_time, last_heartbeat_times[last_heartbeat_index]);
    double phi = -log10(exp(-time_diff / mean));
    return phi;
}

/**
 * Function to update the Phi Accrual Failure Detector parameters.
 * This function updates the mean and variance of the heartbeat arrival times
 * using a method similar to the Welford's online algorithm.
 * @param new_time The time of the new heartbeat
 */
void update_phi_accrual_failure_detector(time_t new_time) {
    // Calculate the time difference between the new heartbeat and the previous one
    double time_diff = difftime(new_time, last_heartbeat_times[last_heartbeat_index]);

    // Update the rolling mean and variance for the inter-arrival times
    // The formula used here for the mean is a simplification of the Welford's method for calculating variance
    // mean(new) = mean(old) + (new_value - mean(old)) / sample_size
    double old_mean = mean;
    mean = mean + (time_diff - mean) / WINDOW_SIZE;

    // The formula for variance uses Welford's online algorithm
    // variance(new) = variance(old) + (new_value - mean(old)) * (new_value - mean(new))
    variance = variance + (time_diff - old_mean) * (time_diff - mean);

    // Update the circular buffer index and value
    last_heartbeat_index = (last_heartbeat_index + 1) % WINDOW_SIZE;
    last_heartbeat_times[last_heartbeat_index] = new_time;
}

/**
 * Function that sends a heartbeat message to the server and updates the
 * Phi Accrual Failure Detector parameters.
 * @param socket The socket to use for sending the message
 */
void send_heartbeat(void *socket, const char *group) {
    time_t current_time = time(NULL);

    // Calculate the phi value based on the current time
    double phi = calculate_phi(current_time);

    // If it's the first heartbeat, or the phi value exceeds the threshold, send a heartbeat
    if (last_heartbeat_times[last_heartbeat_index] == 0 || phi > PHI_THRESHOLD) {
        char heartbeat_message[] = "HB";

        // Send the heartbeat message
        if (zmq_send_group(socket, group, heartbeat_message, 0) < 0) {
            logger(LOG_LEVEL_ERROR, "Failed to send heartbeat: %s", zmq_strerror(errno));
        } else {
            logger(LOG_LEVEL_INFO2, "Heartbeat sent");
        }

        // Update the parameters of the Phi Accrual Failure Detector
        update_phi_accrual_failure_detector(current_time);
    } else {
        logger(LOG_LEVEL_INFO2, "Heartbeat not sent, phi: %f", phi);
    }
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
