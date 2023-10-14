#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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
    printf("\n[WARNING]: Keyboard interruption received (Ctrl+C)\n"); // Translated into English
    exit(0);
}

int main(void) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function
    // Create a new ZeroMQ context. This is necessary for almost all ZeroMQ operations.
    void *ctx = zmq_ctx_new();
    assert(ctx);  // Ensure the context is not NULL (i.e., it has been successfully created).

    // Create a new ZeroMQ socket of type DGRAM. This socket type is used for sending and receiving UDP datagrams.
    void *receiver = zmq_socket(ctx, ZMQ_DGRAM);
    assert(receiver);  // Verify that the socket creation was successful.

    // Bind the socket to the specified local address and port. In this case, it will listen on all network interfaces at port 5556.
    assert(zmq_bind(receiver, "udp://127.0.0.1:5556") == 0);  // Verify that the binding was successful.

    // Infinite loop to receive messages.
    while (!interrupted) {
        char address[256];  // Buffer for the address of the incoming message
        char message[256];  // Buffer for the body of the incoming message

        // Receive the address and then the message body.
        assert(zmq_recv(receiver, address, 255, 0) != -1);

        assert(zmq_recv(receiver, message, 255, 0) != -1);

        // Print the received message.
        printf("Received: %s\n", message);

        const char *response = "Yes, there is!";  // The response that will be sent back.

        // Send the address first and then the response message.
        assert(zmq_send(receiver, address, strlen(address), ZMQ_SNDMORE) != -1);
        assert(zmq_send(receiver, response, strlen(response), 0) != -1);
    }

    // Close the socket and destroy the ZeroMQ context. In a real application, these calls are important for proper resource management.
    // Note: These lines are not reached in this code due to the infinite while(1) loop. You should handle an exit signal or another termination condition to properly exit the loop and perform cleanup.
    zmq_close(receiver);
    zmq_ctx_destroy(ctx);

    return 0;
}
