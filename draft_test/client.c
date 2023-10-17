#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <pthread.h>
#include "../common/zhelpers.h"
#include "../common/utils.h"


int interrupted_ = 0;


void *responder_thread(void *arg) {
    void *socket = (void *) arg;
    while (!interrupted_) {
        char buffer[1024];
        int rc = zmq_recv(socket, buffer, 1023, 0);
        if (rc == -1 && errno == EAGAIN) {
            // Timeout occurred
            continue;
        }

        buffer[rc] = '\0'; // Null-terminate the string
        printf("Message received from server [RESPONDER]: %s\n", buffer);
    }
    return NULL;
}

int main(void) {

    sleep(3);    // wait for server to start
    printf("Client started\n");

    int rc;


    void *context = create_context();

    // RADIO for CLIENT
    void *radio = create_socket(context, ZMQ_RADIO, "udp://127.0.0.1:5555", 1000, NULL);

    // -----------------------------------------------------------------------------------------------------------------
    // DISH for RESPONDER
    // -----------------------------------------------------------------------------------------------------------------

    void *dish = create_socket(context, ZMQ_DISH, "udp://127.0.0.1:5556", 1000, "REP");

    pthread_t responder;
    pthread_create(&responder, NULL, responder_thread, dish);
    // -----------------------------------------------------------------------------------------------------------------
    timespec send_time = getCurrentTime();


    while (1) {
        const char *msg = "Hello";

        rc = zmq_send_group(radio, "GRP", msg, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }
        printf("Sent from client [MAIN]: %s (time spent since last message: %f)\n", msg,
               getElapsedTime(send_time, NULL));

        send_time = getCurrentTime();


        sleep(5); // Send a message every second
    }

    interrupted_ = 1;
    pthread_join(responder, NULL);
    zmq_close(dish);

    zmq_close(radio);
    zmq_ctx_destroy(context);

    return 0;
}
