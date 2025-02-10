#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include "green_threads.h"
#include "detect_deadlock.h"
#include "green_rdlock.h"

#define MAX_SLEEP 3  // Maximum sleep time to increase deadlock probability

green_rwlock_t rwlockA, rwlockB;

// Thread 1 function
void *thread_function1(void *arg) {
    while (1) {
        int sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 1] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 1] Trying to lock rwlockA (holding none)\n");
        green_rwlock_wrlock(&rwlockA);  // Acquire write lock on rwlockA
        printf("[Thread 1] Acquired rwlockA\n");

        sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 1] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 1] Trying to lock rwlockB (holding rwlockA)\n");
        green_rwlock_rdlock(&rwlockB);  // Attempt to acquire read lock on rwlockB
        printf("[Thread 1] Acquired rwlockB (this should never be reached in a deadlock)\n");

        green_rwlock_unlock(&rwlockB);
        green_rwlock_unlock(&rwlockA);
    }
    return NULL;
}

// Thread 2 function
void *thread_function2(void *arg) {
    while (1) {
        int sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 2] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 2] Trying to lock rwlockA (holding rwlockB)\n");
        green_rwlock_wrlock(&rwlockB);  // Attempt to acquire write lock on rwlockA
        printf("[Thread 2] Acquired rwlockB\n");

        sleep_time = rand() % MAX_SLEEP;
        printf("[Thread 2] Sleeping for %d seconds...\n", sleep_time);
        sleep(sleep_time);

        printf("[Thread 2] Trying to lock rwlockB (holding none)\n");
        green_rwlock_rdlock(&rwlockA);  // Acquire read lock on rwlockB
        printf("[Thread 2] Acquired rwlockB (this should never be reached in a deadlock)\n");

        green_rwlock_unlock(&rwlockA);
        green_rwlock_unlock(&rwlockB);
    }
    return NULL;
}

int main() {
    ult_t *thread1, *thread2;

    ult_init();

    // Initialize rwlocks
    green_rwlock_init(&rwlockA);
    green_rwlock_init(&rwlockB);

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
