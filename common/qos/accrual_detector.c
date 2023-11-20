#include "accrual_detector.h"
#include "core/logger.h"
#include <zmq.h>
#include "core/config.h"
#include "core/zhelpers.h"
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


#define PHI_THRESHOLD 1                    // Threshold of the phi value to consider a disconnection
#define WINDOW_SIZE 10                      // Size of the window for the samples, used in the rolling mean and variance
#define INITIAL_HEARTBEAT_INTERVAL 0.0000000001     // Initial heartbeat interval (in seconds)
#define MAX_HEARTBEAT_INTERVAL 1            // Maximum heartbeat interval (in seconds)
#define MIN_HEARTBEAT_INTERVAL 0.1          // Minimum heartbeat interval   (in seconds)
#define INCREASE_FACTOR 0.1                 // Factor to increase the heartbeat interval when there is no message loss

// Global variables to store the times of the last heartbeats
time_t HEARTBEATS_WINDOW[WINDOW_SIZE];
int last_heartbeat_index = 0;
double g_mean = INITIAL_HEARTBEAT_INTERVAL;
double g_variance = 0.0;
double lost_message_rate[WINDOW_SIZE]; // The rate of lost messages
int lost_message_rate_index = 0;
double consecutive_zeros_count = 0.0;

bool log_heartbeat = true;

/**
 * Function to update the Phi Detector parameters, based on the number of missed messages.
 * This function updates the mean and variance of the heartbeat arrival times.
 * @param missed_count The number of missed messages
 */
void update_phi_detector(size_t missed_count) {
//    logger(LOG_LEVEL_DEBUG, "Updating Phi Detector, missed count: %zu", missed_count);
    time_t current_time = time(NULL);

    // Update the times of the last heartbeats and the lost message rate
    HEARTBEATS_WINDOW[last_heartbeat_index] = current_time;
    lost_message_rate[lost_message_rate_index] = (double) missed_count;
    last_heartbeat_index = (last_heartbeat_index + 1) % WINDOW_SIZE;
    lost_message_rate_index = (lost_message_rate_index + 1) % WINDOW_SIZE;

    // Calculate the average lost message rate and the average time difference
    double total_lost = 0.0;
    double total_time_diff = 0.0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
        total_lost += lost_message_rate[i];

        // Calculate time differences between consecutive heartbeats
        if (i > 0) {
            double time_diff = difftime(HEARTBEATS_WINDOW[i], HEARTBEATS_WINDOW[i - 1]);
            total_time_diff += time_diff;
        }
    }
    double average_lost_rate = total_lost / WINDOW_SIZE;
    double average_time_diff = total_time_diff / (WINDOW_SIZE - 1);

    // Calculate the variance
    double variance_sum = 0.0;
    for (int i = 1; i < WINDOW_SIZE; i++) {
        double time_diff = difftime(HEARTBEATS_WINDOW[i], HEARTBEATS_WINDOW[i - 1]);
        variance_sum += pow(time_diff - average_time_diff, 2);
    }
    g_variance = variance_sum / (WINDOW_SIZE - 1);

    // Adjust the heartbeat interval based on the average lost rate and variance
    if (average_lost_rate > 0) {
        // Decrease the interval due to message loss
        g_mean = fmax(MIN_HEARTBEAT_INTERVAL, g_mean * (1 - average_lost_rate) * (1 + sqrt(g_variance)));
        // Reset the consecutive zero count
        consecutive_zeros_count = 0.0;
    } else {
        // Apply exponential decay to the consecutive zero count
        consecutive_zeros_count = (consecutive_zeros_count * 0.9) + 1.0;
        // Increase the interval due to no message loss, moderating based on g_variance and decayed zero count
        double increase_factor =
                1.0 + (INCREASE_FACTOR * fmin(consecutive_zeros_count, 10)); // Caps the increase factor
        g_mean = fmin(MAX_HEARTBEAT_INTERVAL, g_mean * increase_factor / (1 + sqrt(g_variance)));
    }
}

double p_later(double time_diff) {
    double m, v, ret;

    m = g_mean;
    v = g_variance;
    ret = 0.5 * erfc((time_diff - m) / v * M_SQRT2);

    return ret;
}

/**
 * Function to calculate the Phi value based on the current time.
 * The formula used for phi is derived from the exponential distribution and is used to
 * quantify the suspicion level of a node failure.
 * Phi is calculated as: phi = -log10(exp(-(time since last heartbeat)/mean))
 * @param current_time The current time
 * @return The calculated phi value
 */
double calculate_phi(time_t current_time) {
    double time_diff = difftime(current_time, HEARTBEATS_WINDOW[last_heartbeat_index]);
    double probability_later = p_later(time_diff);

    double phi = -log10(probability_later);

    logger(LOG_LEVEL_INFO, "Phi: %8.4lf, Plater: %8.4lf, Mean: %8.4lf, Variance: %8.4lf", phi, probability_later, g_mean, g_variance);
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
    double time_diff = difftime(new_time, HEARTBEATS_WINDOW[last_heartbeat_index]);

    // Update the rolling mean and variance for the inter-arrival times
    // The formula used here for the mean is a simplification of the Welford's method for calculating variance
    // mean(new) = mean(old) + (new_value - mean(old)) / sample_size
    double old_mean = g_mean;
    g_mean = g_mean + (time_diff - g_mean) / WINDOW_SIZE;

    // The formula for variance uses Welford's online algorithm
    // variance(new) = variance(old) + (new_value - mean(old)) * (new_value - mean(new))
    g_variance = g_variance + (time_diff - old_mean) * (time_diff - g_mean);

    // Update the circular buffer index and value
    last_heartbeat_index = (last_heartbeat_index + 1) % WINDOW_SIZE;
    HEARTBEATS_WINDOW[last_heartbeat_index] = new_time;
}

/**
 * Function that sends a heartbeat message to the server and updates the
 * Phi Accrual Failure Detector parameters.
 * @param socket The socket to use for sending the message
 */
bool send_heartbeat(void *socket, const char *group, bool force_send) {
    time_t current_time = time(NULL);

    if (HEARTBEATS_WINDOW[last_heartbeat_index] == 0) {
        // If it's the first heartbeat, set the time of the last heartbeat to the current time
        HEARTBEATS_WINDOW[last_heartbeat_index] = current_time;
        return 0;
    }

    // Calculate the phi value based on the current time
    double phi = calculate_phi(current_time);

    // If it's the first heartbeat, or the phi value exceeds the threshold, send a heartbeat
    if (HEARTBEATS_WINDOW[last_heartbeat_index] == 0 || phi > PHI_THRESHOLD || force_send) {
        char heartbeat_message[] = "HB";


        // Send the heartbeat message
        if (socket != NULL) {
            if (zmq_send_group(socket, group, heartbeat_message, 0) < 0) {
                logger(LOG_LEVEL_ERROR, "Failed to send heartbeat: %s", zmq_strerror(errno));
            } else {
                if (log_heartbeat) logger(LOG_LEVEL_INFO2, "Heartbeat sent (phi: %f)", phi);
            }

        } else {
            if (log_heartbeat) logger(LOG_LEVEL_INFO2, "Heartbeat sent (phi: %f)", phi);
        }

        // Update the parameters of the Phi Accrual Failure Detector
        update_phi_accrual_failure_detector(current_time);
        return true;
    } else {
        if (log_heartbeat) logger(LOG_LEVEL_INFO2, "Heartbeat not sent, phi: %f", phi);
    }
    return false;
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
