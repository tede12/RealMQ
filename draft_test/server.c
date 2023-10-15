#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()

int main(void) {

    int rc;
    void *ctx = zmq_ctx_new();
    assert(ctx);

    void *dish = zmq_socket(ctx, ZMQ_DISH);
    assert(dish);
    int timeout = 2000;
    zmq_setsockopt(dish, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));

    assert(zmq_bind(dish, "udp://127.0.0.1:5556") == 0);

    rc = zmq_join(dish, "GRP");
    assert(rc == 0);

    while (1) {
        char buffer[1024];
        rc = zmq_recv(dish, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            printf("Timeout occurred\n");
        }

        buffer[rc] = '\0'; // Null-terminate the string
        printf("Received: %s\n", buffer);
    }

    zmq_close(dish);
    zmq_ctx_destroy(ctx);
    return 0;
}
