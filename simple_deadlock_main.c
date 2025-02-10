#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "green_threads.h"
#include "detect_deadlock.h"
#include "green_mutex.h"

#define MAX_SLEEP 3  // Maximum sleep time to increase deadlock probability

green_mutex_t mutexA, mutexB;

// Thread 1 function
void *thread_function1(void *arg) {
    while (1) {
        int sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 1] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 1] Trying to lock mutexA (holding none)\n");
        green_mutex_lock(&mutexA);
        printf("[Thread 1] Acquired mutexA\n");

        sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 1] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 1] Trying to lock mutexB (holding mutexA)\n");
        green_mutex_lock(&mutexB);
        printf("[Thread 1] Acquired mutexB (this should never be reached in a deadlock)\n");

        green_mutex_unlock(&mutexB);
        green_mutex_unlock(&mutexA);
    }
    return NULL;
}

// Thread 2 function
void *thread_function2(void *arg) {
    while (1) {
        int sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 2] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 2] Trying to lock mutexB (holding none)\n");
        green_mutex_lock(&mutexB);
        printf("[Thread 2] Acquired mutexB\n");

        sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 2] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 2] Trying to lock mutexA (holding mutexB)\n");
        green_mutex_lock(&mutexA);
        printf("[Thread 2] Acquired mutexA (this should never be reached in a deadlock)\n");

        green_mutex_unlock(&mutexA);
        green_mutex_unlock(&mutexB);
    }
    return NULL;
}

int main() {
    ult_t *thread1, *thread2;

    ult_init();

    // Initialize mutexes
    green_mutex_init(&mutexA);
    green_mutex_init(&mutexB);

    // Register deadlock detection signal (CTRL+Z to check)
    signal(SIGTSTP, detect_deadlock);

    // Create threads
    ult_create(&thread1, thread_function1, NULL);
    ult_create(&thread2, thread_function2, NULL);

    // Join threads
    ult_join(thread1);
    ult_join(thread2);

    return 0;
}