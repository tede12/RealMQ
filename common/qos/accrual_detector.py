import time
import math
import os

root_path = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
print(f"[root_path: {root_path}]\n\n")
logger_path = os.path.join(root_path, 'logger.log')


class Logger:
    def __init__(self):
        self.logger = open(logger_path, 'w')
        self.logger.write("")

    def log(self, msg):
        print(msg)
        self.logger.write(f"{msg}\n")
        self.logger.flush()


logger = Logger()

# Initial heartbeat interval in seconds (adjust as needed)
PHI_THRESHOLD = 4.0  # Threshold of the phi value to consider a disconnection
WINDOW_SIZE = 10  # Size of the window for the samples, used in the rolling mean and variance
INITIAL_HEARTBEAT_INTERVAL = 1.0  # Initial heartbeat interval (in seconds)
MAX_HEARTBEAT_INTERVAL = 1  # Maximum heartbeat interval (in seconds)
MIN_HEARTBEAT_INTERVAL = 0.1  # Minimum heartbeat interval (in seconds)
INCREASE_FACTOR = 0.1  # Factor to increase the heartbeat interval
MAX_VARIANCE = 1.0  # Maximum variance

# Global variables to store the times of the last heartbeats
HEARTBEATS_WINDOW = [0] * WINDOW_SIZE
last_heartbeat_index = 0
g_mean = INITIAL_HEARTBEAT_INTERVAL
g_variance = 0.1  # Initial variance (adjust as needed)
lost_message_rate = [0] * WINDOW_SIZE  # The rate of lost messages
lost_message_rate_index = 0
consecutive_zeros_count = 0.0

log_heartbeat = True


def update_phi_detector(missed_count):
    global last_heartbeat_index, lost_message_rate_index, consecutive_zeros_count, g_mean, g_variance
    current_time = time.time()

    # Update the times of the last heartbeats and the lost message rate
    HEARTBEATS_WINDOW[last_heartbeat_index] = current_time
    lost_message_rate[lost_message_rate_index] = missed_count
    last_heartbeat_index = (last_heartbeat_index + 1) % WINDOW_SIZE
    lost_message_rate_index = (lost_message_rate_index + 1) % WINDOW_SIZE

    # Calculate the average lost message rate and the average time difference
    total_lost = 0.0
    total_time_diff = 0.0
    for i in range(WINDOW_SIZE):
        total_lost += lost_message_rate[i]

        # Calculate time differences between consecutive heartbeats
        if i > 0:
            time_diff = HEARTBEATS_WINDOW[i] - HEARTBEATS_WINDOW[i - 1]
            total_time_diff += time_diff

    average_lost_rate = total_lost / WINDOW_SIZE
    g_mean = total_time_diff / (WINDOW_SIZE - 1)

    # Calculate the variance
    variance_sum = 0.0
    for i in range(1, WINDOW_SIZE):
        time_diff = HEARTBEATS_WINDOW[i] - HEARTBEATS_WINDOW[i - 1]
        variance_sum += pow(time_diff - g_mean, 2)

    g_variance = variance_sum / (WINDOW_SIZE - 1)

    # Boundary checks for variance
    if g_variance > MAX_VARIANCE:
        g_variance = MAX_VARIANCE

    # Adjust the heartbeat interval based on the average lost rate and variance
    if average_lost_rate > 0:
        g_mean = max(MIN_HEARTBEAT_INTERVAL, g_mean * (1 - average_lost_rate) * (1 + math.sqrt(g_variance)))
    else:
        # Increase the interval due to no message loss, moderating based on g_variance and decayed zero count
        increase_factor = 1.0 + (INCREASE_FACTOR * min(consecutive_zeros_count, 10))
        g_mean = min(MAX_HEARTBEAT_INTERVAL, g_mean * increase_factor / (1 + math.sqrt(g_variance)))

    if average_lost_rate == 0:
        consecutive_zeros_count += 1
    else:
        consecutive_zeros_count = 0

    if log_heartbeat:
        logger.log("Mean: %8.4lf, Variance: %8.4lf, Lost rate: %8.4lf, Consecutive zeros: %8.4lf" % (
            g_mean, g_variance, average_lost_rate, consecutive_zeros_count))


def p_later(time_diff):
    global g_mean, g_variance

    m = g_mean
    v = g_variance
    ret = 0.5 * math.erfc((time_diff - m) / v * math.sqrt(2))
    return ret


def calculate_phi(current_time):
    time_diff = current_time - HEARTBEATS_WINDOW[last_heartbeat_index]
    probability_later = p_later(time_diff)

    if probability_later <= 0.0000001:
        probability_later = 0.0000001

    phi = -math.log10(probability_later)
    logger.log(
        "Phi: %8.4lf, Plater: %8.4lf, Mean: %8.4lf, Variance: %8.4lf" % (phi, probability_later, g_mean, g_variance))
    return phi


def update_phi_accrual_failure_detector(new_time):
    global g_mean, g_variance, last_heartbeat_index, lost_message_rate_index, consecutive_zeros_count

    time_diff = new_time - HEARTBEATS_WINDOW[last_heartbeat_index]

    old_mean = g_mean
    g_mean = g_mean + (time_diff - old_mean) * (time_diff - g_mean)

    last_heartbeat_index = (last_heartbeat_index + 1) % WINDOW_SIZE
    HEARTBEATS_WINDOW[last_heartbeat_index] = new_time


def send_heartbeat(socket=None, group=None, force_send=False):
    current_time = time.time()
    if force_send:
        logger.log("DEFAULTS, PHI_THRESHOLD: %8.4lf, WINDOW_SIZE: %d, INCREASE_FACTOR: %8.4lf" % (
            PHI_THRESHOLD, WINDOW_SIZE, INCREASE_FACTOR))

    if HEARTBEATS_WINDOW[last_heartbeat_index] == 0:
        # If it's the first heartbeat, set the time of the last heartbeat to the current time
        HEARTBEATS_WINDOW[last_heartbeat_index] = current_time
        return False

    # Calculate the phi value based on the current time
    phi = calculate_phi(current_time)

    # If it's the first heartbeat, or the phi value exceeds the threshold, send a heartbeat
    if HEARTBEATS_WINDOW[last_heartbeat_index] == 0 or phi > PHI_THRESHOLD or force_send:
        heartbeat_message = "HB"
        if socket:
            logger.log(f"Heartbeat sent (phi: %f)" % phi)
        else:
            logger.log(f"Heartbeat sent (phi: %f)" % phi)

        # Update the parameters of the failure detector
        update_phi_accrual_failure_detector(current_time)
        return True
    else:
        logger.log(f"Heartbeat not sent, phi: %f" % phi)

    return False


if __name__ == '__main__':
    print("Starting")
