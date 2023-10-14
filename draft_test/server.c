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
    printf("\n[WARNING]: Keyboard interruption received (Ctrl+C)\n");
    exit(0);
}

int main(void) {
    signal(SIGINT, handle_interrupt); // Register the interruption handling function
    // Create a new ZeroMQ context. This is necessary for almost all ZeroMQ operations.
    void *ctx = zmq_ctx_new();
    assert(ctx);  // Ensure the context is not NULL (i.e., it has been created successfully).

    // Create a new ZeroMQ socket of type DGRAM. This type of socket is used for sending and receiving UDP datagrams.
    void *receiver = zmq_socket(ctx, ZMQ_DGRAM);
    assert(receiver);  // Verify that the socket was successfully created.

    // Bind the socket to the specified local address and port. In this case, it will listen on all network interfaces at port 5556.
    int rc = zmq_bind(receiver, "udp://*:5556");
    assert(rc == 0);  // Verify that the binding was successful.

    // Infinite loop to receive messages.
    while (1) {
        // Prepare a zmq_msg_t structure to receive the address of the incoming message.
        zmq_msg_t address;
        zmq_msg_init(&address);  // Initialize the message to a valid but empty state.
        // Receive the message address. This is a message fragment containing the sender's address.
        rc = zmq_msg_recv(&address, receiver, 0);
        assert(rc != -1);  // Verify that the reception was successful.

        // Prepare a zmq_msg_t structure to receive the message body.
        zmq_msg_t message;
        zmq_msg_init(&message);  // Initialize the message to a valid but empty state.

        // Receive the message body.
        rc = zmq_msg_recv(&message, receiver, 0);
        assert(rc != -1);  // Verify that the reception was successful.

        // Print the received message.
        printf("Received: %s\n", (char *) zmq_msg_data(&message));

        // Prepare the response message.
        zmq_msg_t reply;
        const char *response = "Yes, there is!";  // The response that will be sent back.
        // Initialize the response message with the content of the response.
        zmq_msg_init_size(&reply, strlen(response));
        // Copy the response into the message.
        memcpy(zmq_msg_data(&reply), response, strlen(response));

        // Send the address first and then the response message. Using ZMQ_SNDMORE indicates that there are more message fragments to send.
        zmq_msg_send(&address, receiver, ZMQ_SNDMORE);
        zmq_msg_send(&reply, receiver, 0);

        // Close the messages after sending them to free resources.
        zmq_msg_close(&address);
        zmq_msg_close(&message);
        zmq_msg_close(&reply);
    }

    // Close the socket and destroy the ZeroMQ context. In a real application, these calls are important for proper resource management.
    // Note: these lines are not reached in this code due to the infinite while(1) loop. You should handle an exit signal or another termination condition to properly exit the loop and perform cleanup.
    zmq_close(receiver);
    zmq_ctx_destroy(ctx);

    return 0;
}
