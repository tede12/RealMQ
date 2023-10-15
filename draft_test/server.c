#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

// Function to send a message with a group using zmq_send-like parameters
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


int main(void) {

    int rc;
    void *context = zmq_ctx_new();
    assert(context);

    // DISH for SERVER
    void *receiver = zmq_socket(context, ZMQ_DISH);
    assert(receiver);
    // Set timeout for receive operations
    assert(zmq_bind(receiver, "udp://127.0.0.1:5555") == 0);
    int timeout = 2000;
    zmq_setsockopt(receiver, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    // Join the group
    rc = zmq_join(receiver, "GRP");
    assert(rc == 0);
    // -----------------------------------------------------------------------------------------------------------------
    // RADIO for RESPONDER
    void *sender = zmq_socket(context, ZMQ_RADIO);
    assert(sender);
    assert(zmq_connect(sender, "udp://127.0.0.1:5556") == 0);
    zmq_setsockopt(sender, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    // -----------------------------------------------------------------------------------------------------------------

    while (1) {
        char buffer[1024];
        rc = zmq_recv(receiver, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            printf("Timeout occurred\n");
            continue;
        }

        buffer[rc] = '\0'; // Null-terminate the string
        printf("Received from client [MAIN]: %s\n", buffer);

        // Send a reply
        const char *msg = "Message sent from server [RESPONDER]";
        zmq_send_group(sender, "REP", msg, 0);
    }

    zmq_close(sender);
    zmq_close(receiver);
    zmq_ctx_destroy(context);
    return 0;
}
