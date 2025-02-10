#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>
#include "detect_deadlock.h"
#include "green_threads.h"
#include "green_rdlock.h"
#include "green_mutex.h"

#define MAX_THREADS 100

// Detect cycles in the thread dependency graph
bool detect_cycle(ult_t *thread, ult_t *visited[], int index) {
    if (thread == NULL) return false;

    // Check if the thread is already in the visited list (cycle detected)
    for (int i = 0; i < index; i++) {
        if (visited[i] == thread) {
            return true; // Cycle detected
        }
    }

    // Mark the current thread as visited
    visited[index] = thread;

    // Recursively check dependencies
    return detect_cycle(thread->waiting_for, visited, index + 1);
}

// Detect and display deadlocks
void detect_deadlock() {
    ult_t *visited[MAX_THREADS]; // Track visited nodes for cycle detection
    printf("Signal received: Checking for deadlocks...\n");
    int deadlock_detected = 0;

    // Reset visited array
    for (int i = 0; i < MAX_THREADS; i++) {
        visited[i] = NULL;
    }

    // Check all threads in `thread_queue`
    for (ult_t *thread = thread_queue; thread != NULL; thread = thread->next) {
        if (detect_cycle(thread, visited, 0)) {
            printf("Deadlock detected: Thread %p is part of a cycle!\n", thread);
            deadlock_detected = 1;
        }
        // Reset visited list
        for (int i = 0; i < MAX_THREADS; i++) {
            visited[i] = NULL;
        }
    }

    // Check threads in mutex waiting lists
    for (green_mutex_t *mutex = all_mutexes; mutex != NULL; mutex = mutex->next) {
        for (ult_t *thread = mutex->waiting_list; thread != NULL; thread = thread->next) {
            if (detect_cycle(thread, visited, 0)) {
                printf("Deadlock detected: Thread %p is part of a cycle in mutex waitlist!\n", thread);
                deadlock_detected = 1;
            }
            // Reset visited list
            for (int i = 0; i < MAX_THREADS; i++) {
                visited[i] = NULL;
            }
        }
    }

    // Check threads in rwlock waiting lists
    for (green_rwlock_t *rwlock = all_rwlocks; rwlock != NULL; rwlock = rwlock->next) {
        // Check waiting writers
        for (ult_t *thread = rwlock->waiting_writers; thread != NULL; thread = thread->next) {
            if (detect_cycle(thread, visited, 0)) {
                printf("Deadlock detected: Thread %p is part of a cycle in rwlock writer waitlist!\n", thread);
                deadlock_detected = 1;
            }
            // Reset visited list
            for (int i = 0; i < MAX_THREADS; i++) {
                visited[i] = NULL;
            }
        }

        // Check waiting readers
        for (ult_t *thread = rwlock->waiting_readers; thread != NULL; thread = thread->next) {
            if (detect_cycle(thread, visited, 0)) {
                printf("Deadlock detected: Thread %p is part of a cycle in rwlock reader waitlist!\n", thread);
                deadlock_detected = 1;
            }
            // Reset visited list
            for (int i = 0; i < MAX_THREADS; i++) {
                visited[i] = NULL;
            }
        }
    }

    if (!deadlock_detected) {
        printf("No deadlocks detected.\n");
    } else {
        printf("Terminating due to deadlock detection!\n");
        exit(1);
    }
}

