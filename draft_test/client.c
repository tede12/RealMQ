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
    void *ctx = zmq_ctx_new();
    assert(ctx);

    void *radio = zmq_socket(ctx, ZMQ_RADIO);
    assert(radio);
    assert(zmq_connect(radio, "udp://127.0.0.1:5556") == 0);
    int timeout = 1000;
    zmq_setsockopt(radio, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    while (1) {
        const char *group = "GRP";
        const char *msg = "Hello";

        int rc = zmq_send_group(radio, group, msg, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }

        sleep(1); // Send a message every second
    }

    zmq_close(radio);
    zmq_ctx_destroy(ctx);
    return 0;
}
