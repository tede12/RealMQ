#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>
#include <time.h>
#include <json-c/json.h>
#include <sys/time.h>

#define ADDRESS "tcp://127.0.0.1:5555"
#define NUM_THREADS 100
#define NUM_MESSAGES 1000

double getMilliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec * 1000.0 + (double) tv.tv_usec / 1000.0;
}

// Function to calculate the current time in milliseconds
long long current_time_millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long) (ts.tv_sec) * 1000 + (long long) (ts.tv_nsec) / 1000000;
}

// Function executed by client threads
void *client_thread(void *thread_id) {
    int thread_num = *(int *) thread_id;
    void *context = zmq_ctx_new();
    void *socket = zmq_socket(context, ZMQ_PUB);
    zmq_connect(socket, ADDRESS);

    for (int i = 0; i < NUM_MESSAGES; i++) {
        // Create the message
        char message[256];
        sprintf(message, "Thread %d - Message %d", thread_num, i);

        // Calculate the send time
        long long send_time = current_time_millis();

        // Create a JSON object for the message
        json_object *json_msg = json_object_new_object();

        // use unique id for each message based on timestamp
        json_object_object_add(json_msg, "id", json_object_new_double(getMilliseconds()));
        json_object_object_add(json_msg, "message", json_object_new_string(message));
        json_object_object_add(json_msg, "send_time", json_object_new_int64(send_time));

        // Send the JSON message to the server
        const char *json_str = json_object_to_json_string(json_msg);
        zmq_send(socket, json_str, strlen(json_str), 0);

        // Measure the arrival time
        zmq_recv(socket, message, sizeof(message), 0);

        printf("Received message: %s\n", message);

        // long long end_time = current_time_millis();

        // received message...
    }

    zmq_close(socket);
    zmq_ctx_destroy(context);
    return NULL;
}

int main() {
    double start_time = getMilliseconds();
    printf("Start Time: %.3f milliseconds\n", start_time);

    // Client threads
    pthread_t clients[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&clients[i], NULL, client_thread, &thread_ids[i]);
    }

    // Wait for client threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(clients[i], NULL);
    }

    printf("Execution Time: %.3f milliseconds\n", getMilliseconds() - start_time);
    return 0;
}
