#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Flag to indicate if a keyboard interruption has been received
volatile sig_atomic_t interrupted = 0;

/**
 * @brief Handle the keyboard interruption (Ctrl+C)
 * @param signal
 */
void handle_interrupt(int sig) {
    (void) sig; // Avoid unused parameter warning
    interrupted = 1;
    printf("\n[WARNING]: Keyboard interruption received (Ctrl+C)\n");
    exit(0);
}

int main(void) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function

    // Create a new ZeroMQ context. This is the first step in working with ZeroMQ and is needed to manage sockets.
    void *ctx = zmq_ctx_new();
    assert(ctx);  // Ensure the context was successfully created.

    // Create a new socket of type DGRAM. It's used for sending and receiving UDP datagrams.
    void *sender = zmq_socket(ctx, ZMQ_DGRAM);
    assert(sender);  // Ensure the socket was successfully created.

    // Bind the socket to the local address and port. The client will listen on port 5557.
    int rc = zmq_bind(sender, "udp://127.0.0.1:5555");
    assert(rc == 0);  // Ensure the binding was successful.

    // Prepare the message and the destination address.
    const char *msg = "Is someone there?";  // The message to send.
    const char *address = "127.0.0.1:5556";  // The server's address (to send messages to).

    while (!interrupted) {
        // First, send the address and then the message body to the server.
        assert(zmq_send(sender, address, strlen(address), ZMQ_SNDMORE) != -1);
        assert(zmq_send(sender, msg, strlen(msg), 0) != -1);

        // Prepare to receive a reply from the server.
        char reply_addr[256];  // Buffer for the reply address
        char reply[256];  // Buffer for the reply body
        rc = zmq_recv(sender, reply_addr, 255, 0);
        assert(rc != -1);
        reply_addr[rc] = '\0';  // Ensure it's null-terminated
        rc = zmq_recv(sender, reply, 255, 0);
        assert(rc != -1);
        reply[rc] = '\0';  // Ensure it's null-terminated

        // Print the response received from the server.
        printf("Received: %s\n", reply);

        // Sleep for 2 seconds
        sleep(2);
    }

    // Close the socket and destroy the ZeroMQ context. These are important steps to ensure all resources are properly cleaned up.
    zmq_close(sender);
    zmq_ctx_destroy(ctx);

    return 0;  // End the program.
}
