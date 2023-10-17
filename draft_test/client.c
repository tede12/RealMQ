#include <zmq.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> // for sleep()
#include <pthread.h>
#include "../common/zhelpers.h"
#include "../common/utils.h"
#include "../common/qos.h"


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
        delete_message_ids_from_buffer(buffer);
//        printf("Message received from server [RESPONDER]: %s\n", buffer);
        printf("Message received from server [RESPONDER] with ids\n");

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

    void *context_2 = create_context();
    void *dish = create_socket(context_2, ZMQ_DISH, "udp://127.0.0.1:5556", 1000, "REP");

    pthread_t responder;
    pthread_create(&responder, NULL, responder_thread, dish);
    // -----------------------------------------------------------------------------------------------------------------
    timespec send_time = getCurrentTime();


    while (1) {
        // Create a message ID
        double recv_time = getCurrentTimeValue(NULL);
        char *msg_id = (char *) malloc(20 * sizeof(char));
        sprintf(msg_id, "%f", recv_time);

        // QoS - Send a heartbeat
        send_heartbeat(radio, "GRP");

        rc = zmq_send_group(radio, "GRP", msg_id, 0);
        if (rc == -1) {
            printf("Error in sending message\n");
            break;
        }
        printf("Sent from client [MAIN]: %s (time spent since last message: %f)\n", msg_id,
               getElapsedTime(send_time, NULL));

        send_time = getCurrentTime();


        sleep(1); // Send a message every second
    }

    interrupted_ = 1;
    pthread_join(responder, NULL);
    zmq_close(dish);
    zmq_close(radio);
    zmq_ctx_destroy(context);
    zmq_ctx_destroy(context_2);

    return 0;
}
