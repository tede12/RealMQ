#include "zhelpers.h"
#include "logger.h"

//  Receive 0MQ string from socket and convert into C string
//  Caller must free returned string. Returns NULL if the context
//  is being terminated.
char *s_recv(void *socket) {
    enum {
        cap = 256
    };
    char buffer[cap];
    int size = zmq_recv(socket, buffer, cap - 1, 0);
    if (size == -1)
        return NULL;
    buffer[size < cap ? size : cap - 1] = '\0';

#if (defined (WIN32))
    return strdup (buffer);
#else
    return strndup(buffer, sizeof(buffer) - 1);
#endif

    // remember that the strdup family of functions use malloc/alloc for space for the new string.  It must be manually
    // freed when you are done with it.  Failure to do so will allow a heap attack.
}

//  Convert C string to 0MQ string and send to socket
int s_send(void *socket, char *string) {
    int size = zmq_send(socket, string, strlen(string), 0);
    return size;
}

//  Sends string as 0MQ string, as multipart non-terminal
int s_sendmore(void *socket, char *string) {
    int size = zmq_send(socket, string, strlen(string), ZMQ_SNDMORE);
    return size;
}


//  Sleep for a number of milliseconds
void s_sleep(int msecs) {
#if (defined (WIN32))
    Sleep (msecs);
#else
    struct timespec t;
    t.tv_sec = msecs / 1000;
    t.tv_nsec = (msecs % 1000) * 1000000;
    nanosleep(&t, NULL);
#endif
}


// --------------------------------------------- ZMQ HELPERS FUNCTIONS -------------------------------------------------

void *create_context() {
    void *context = zmq_ctx_new();
    assert(context);
    return context;
}

ProtocolType get_protocol_type(int socket_type) {
    if ((socket_type >= 0) && (socket_type <= 11)) {    // 0 <= TCP <= 11
        return TCP;
    } else if ((socket_type >= 12) && (socket_type <= 20)) {  // 12<= UDP <= 20
        return UDP;
    } else {
        printf("Invalid socket type\n");
        return -1;
    }
}


/**
 * Function to create a new ZMQ socket
 * @param context
 * @param socket_type
 * @param connection
 * @param bind
 * @param timeout
 * @return
 */
void *create_socket(void *context, int socket_type, const char *connection, int timeout, char *socket_group) {
    /* Explanation of all steps for configuring a socket:
    1. TCP/UDP - [Create] a new socket
    2. UDP     - [Connect] or [bind] the socket
    3. TCP/UDP - [Set] the socket options (ex. timeout)
    4. UDP     - [Join] the group (only for receiving messages)
     */

    int rc;
    char common_msg[256];
    snprintf(common_msg, sizeof(common_msg), "[socket with type: %d and connection: %s]", socket_type, connection);

    // 1. Create a new socket
    void *socket = zmq_socket(context, socket_type);
    if (socket == NULL) {
        logger(LOG_LEVEL_ERROR, "Failed to [CREATE] %s", common_msg);
        assert(socket);
    }

    /* Passing socket_group as NULL means that the socket is only used for sending messages
     * Passing socket_group as a string means that the socket is only used for receiving messages,
     * this means that there will be a bind operation
     */

    switch (get_protocol_type(socket_type)) {
        case TCP:
            // 3. Add timeout option to the socket
            zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
            break;

        case UDP:
            if (socket_group == NULL) {
                // 2. Connect is used for sending messages
                rc = zmq_connect(socket, connection);
                if (rc != 0) {
                    logger(LOG_LEVEL_ERROR, "Failed to [CONNECT] %s", common_msg);
                    assert(rc == 0);
                }
            } else {
                // 2. Bind is used for receiving messages, so we also need to join the group (after the setsockopt)
                rc = zmq_bind(socket, connection);
                if (rc != 0) {
                    logger(LOG_LEVEL_ERROR, "Failed to [BIND] %s", common_msg);
                    assert(rc == 0);
                }
            }

            // 3. Add timeout option to the socket
            zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

            // 4. Join the group
            if (socket_group != NULL) {
                rc = zmq_join(socket, (const char *) socket_group);
                if (rc != 0) {
                    logger(LOG_LEVEL_ERROR, "Failed to [JOIN] %s", common_msg);
                    assert(rc == 0);
                }
            }

            break;
        default:
            logger(LOG_LEVEL_ERROR, "Invalid protocol type [%d]", get_protocol_type(socket_type));
            return NULL;
    }

    logger(LOG_LEVEL_INFO, "Successfully created and configured %s", common_msg);
    return socket;
}


/**
 * Function to send a message with a group using zmq_send-like parameters
 * @param socket
 * @param group
 * @param msg
 * @param flags
 * @return
 */
int zmq_send_group(void *socket, const char *group, const char *msg, int flags) {
    zmq_msg_t message;
    zmq_msg_init_size(&message, strlen(msg));
    memcpy(zmq_msg_data(&message), msg, strlen(msg));

    int rc = zmq_msg_set_group(&message, group);
    if (rc != 0) {
        zmq_msg_close(&message);
        return rc;
    }

    rc = zmq_msg_send(&message, socket, flags);
    zmq_msg_close(&message);
    return rc;
}

// ---------------------------------------------------------------------------------------------------------------------