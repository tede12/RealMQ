#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include "../common/zhelpers.h"

int main(void) {
    printf("Server started\n");

    int rc;
    void *context = create_context();

    // DISH for SERVER
    void *dish = create_socket(context, ZMQ_DISH, "udp://127.0.0.1:5555", 2000, "GRP");
    // -----------------------------------------------------------------------------------------------------------------
    // RADIO for RESPONDER
    void *radio = create_socket(context, ZMQ_RADIO, "udp://127.0.0.1:5556", 2000, NULL);
    // -----------------------------------------------------------------------------------------------------------------

    while (1) {
        char buffer[1024];
        rc = zmq_recv(dish, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            continue;
        }

        // Check if prefix of the message is "HB" (heartbeat)
        if (strncmp(buffer, "HB", 2) == 0) {
            // Heartbeat received
            printf("Heartbeat received\n");
            continue;
        }

        buffer[rc] = '\0'; // Null-terminate the string
        printf("Received from client [MAIN]: %s\n", buffer);

        // Send a reply
        const char *msg = "Message sent from server [RESPONDER]";
        zmq_send_group(radio, "REP", msg, 0);
    }

    zmq_close(radio);
    zmq_close(dish);
    zmq_ctx_destroy(context);
    return 0;
}
