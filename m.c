#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "green_threads.h"  // Make sure this header includes ult_init() and other functions

#define NUM_THREADS 10

// Thread function that simply prints the thread ID
void *thread_function(void *arg) {
    int id = *(int *)arg;
    printf("Thread ID: %d\n", id);
    return NULL;
}

/*this won't create any deadlocks*/
int main() {
    // Initialize the green thread library
    ult_init();  // Ensure this is correctly called to initialize the green thread system

    // Create an array of thread pointers and thread IDs
    ult_t *threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        ult_create(&threads[i], thread_function, &thread_ids[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        ult_join(threads[i]);
    }

    return 0;
}
