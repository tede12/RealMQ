#include <stdio.h>
#include <pthread.h>

void* thread_function(void* arg) {
    printf("Salve dal thread!\n");
    return NULL;
}

int main() {
    pthread_t thread_id;

    printf("Creazione del thread...\n");
    pthread_create(&thread_id, NULL, thread_function, NULL);

    pthread_join(thread_id, NULL);

    printf("Thread terminato.\n");
    return 0;
}
