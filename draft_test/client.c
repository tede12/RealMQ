#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

// Flag to indicate if keyboard interruption has been received
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

    // Create a new ZeroMQ context. This is the first step in working with ZeroMQ and is used to manage sockets.
    void *ctx = zmq_ctx_new();
    assert(ctx);  // Ensure that the context has been created successfully.

    // Create a new socket of type DGRAM. It is used for sending and receiving UDP datagrams.
    void *sender = zmq_socket(ctx, ZMQ_DGRAM);
    assert(sender);  // Verify that the socket creation was successful.

    // Bind the socket to the local address and port. The client will listen on port 5557.
    int rc = zmq_bind(sender, "udp://*:5557");
    assert(rc == 0);  // Verify that the binding was successful.

    // Prepare the message and destination address.
    const char *msg = "Is someone there?";  // The message to be sent.
    const char *address = "127.0.0.1:5556";  // The server's address (to send messages).

    while (1) {

        // Create a ZMQ message with the destination address.
        zmq_msg_t addr_msg;
        zmq_msg_init_size(&addr_msg, strlen(address));  // Initialize the message with the size of the address.
        memcpy(zmq_msg_data(&addr_msg), address, strlen(address));  // Copy the address into the message.

        // Create a ZMQ message with the message body.
        zmq_msg_t query_msg;
        zmq_msg_init_size(&query_msg, strlen(msg));  // Initialize the message with the size of the message body.
        memcpy(zmq_msg_data(&query_msg), msg, strlen(msg));  // Copy the message body into the message.

        // Send the address first and then the message body to the server. Using ZMQ_SNDMORE indicates that there are more message frames to send.
        zmq_msg_send(&addr_msg, sender, ZMQ_SNDMORE);  // Send the address with a flag indicating there will be another message frame.
        zmq_msg_send(&query_msg, sender, 0);  // Send the message body.

        // Close the messages to free resources.
        zmq_msg_close(&addr_msg);
        zmq_msg_close(&query_msg);

        // Prepare to receive the response from the server.
        zmq_msg_t reply_addr;  // For the reply message's address.
        zmq_msg_init(&reply_addr);  // Initialize the message.
        rc = zmq_msg_recv(&reply_addr, sender, 0);  // Receive the reply message's address.
        assert(rc != -1);  // Verify that the reception was successful.

        zmq_msg_t reply;  // For the reply message body.
        zmq_msg_init(&reply);  // Initialize the message.
        rc = zmq_msg_recv(&reply, sender, 0);  // Receive the reply message body.
        assert(rc != -1);  // Verify that the reception was successful.

        // Print the response received from the server.
        printf("Received: %s\n", (char *) zmq_msg_data(&reply));  // Convert the message data to a string and print it.

        // Close the reply messages to free resources.
        zmq_msg_close(&reply_addr);
        zmq_msg_close(&reply);

        // Sleep for 2 seconds
        sleep(2);
    }

    // Close the socket and destroy the ZeroMQ context. These are important steps to ensure all resources are cleaned up properly.
    zmq_close(sender);
    zmq_ctx_destroy(ctx);

    return 0;  // Terminate the program.
}
