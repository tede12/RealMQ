#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

int main(void) {

    int rc;
    void *context = zmq_ctx_new();
    assert(context);

    void *receiver = zmq_socket(context, ZMQ_DISH);
    assert(receiver);

    // Set timeout for receive operations
    assert(zmq_bind(receiver, "udp://127.0.0.1:5555") == 0);

    int timeout = 2000;
    zmq_setsockopt(receiver, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));


    rc = zmq_join(receiver, "GRP");
    assert(rc == 0);

    while (1) {
        char buffer[1024];
        rc = zmq_recv(receiver, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            printf("Timeout occurred\n");
        }

        buffer[rc] = '\0'; // Null-terminate the string
        printf("Received: %s\n", buffer);
    }

    zmq_close(receiver);
    zmq_ctx_destroy(context);
    return 0;
}
