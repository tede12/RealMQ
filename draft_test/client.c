#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

int main(void) {
    int rc;

    void *ctx = zmq_ctx_new();
    assert(ctx);

    void *radio = zmq_socket(ctx, ZMQ_RADIO);
    assert(radio);
    assert(zmq_connect(radio, "udp://127.0.0.1:5556") == 0);

    while (1) {
        const char *msg = "Hello";
        zmq_msg_t message;
        zmq_msg_init_size(&message, strlen(msg));
        memcpy(zmq_msg_data(&message), msg, strlen(msg));
        rc = zmq_msg_set_group(&message, "GRP");
        assert(rc == 0);

        rc = zmq_msg_send(&message, radio, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            zmq_msg_close(&message);
            break;
        }

        zmq_msg_close(&message);
        sleep(1); // Send a message every second
    }

    zmq_close(radio);
    zmq_ctx_destroy(ctx);
    return 0;
}
