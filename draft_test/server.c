#include <zmq.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>

int main(void) {
    void *context = zmq_ctx_new();
    void *responder = zmq_socket(context, ZMQ_DGRAM);
    int rc = zmq_bind(responder, "udp://*:5556");
    assert(rc == 0);

    while (1) {
        char buffer[10];
        zmq_recv(responder, buffer, 10, 0);
        printf("Received: %s\n", buffer);
        sleep(1); // Do some 'work'
    }

    return 0;
}
