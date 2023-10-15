#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <pthread.h>


int interrupted = 0;

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


void *responder_thread(void *arg) {
    void *socket = (void *) arg;
    while (!interrupted) {
        char buffer[1024];
        int rc = zmq_recv(socket, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            printf("Timeout occurred\n");
            continue;
        }

        buffer[rc] = '\0'; // Null-terminate the string
        printf("Message received from server [RESPONDER]: %s\n", buffer);
    }
    return NULL;
}

int main(void) {
    int rc;


    void *context = zmq_ctx_new();
    assert(context);

    // RADIO for CLIENT
    void *radio = zmq_socket(context, ZMQ_RADIO);
    assert(radio);
    assert(zmq_connect(radio, "udp://127.0.0.1:5555") == 0);
    int timeout = 1000;
    zmq_setsockopt(radio, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    // -----------------------------------------------------------------------------------------------------------------
    // DISH for RESPONDER
    void *dish = zmq_socket(context, ZMQ_DISH);
    assert(dish);
    assert(zmq_bind(dish, "udp://127.0.0.1:5556") == 0);
    zmq_setsockopt(dish, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
    // Join a group
    assert(zmq_join(dish, "REP") == 0);

    pthread_t responder;
    pthread_create(&responder, NULL, responder_thread, dish);
    // -----------------------------------------------------------------------------------------------------------------

    while (1) {
        const char *group = "GRP";
        const char *msg = "Hello";

        rc = zmq_send_group(radio, group, msg, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }
        printf("Sent from client [MAIN]: %s\n", msg);

        sleep(1); // Send a message every second
    }

    interrupted = 1;
    pthread_join(responder, NULL);
    zmq_close(dish);

    zmq_close(radio);
    zmq_ctx_destroy(context);

    return 0;
}
