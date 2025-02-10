#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "green_threads.h"
#include "green_mutex.h"

// Signal mask to block all signals
static sigset_t block_all_signals;
green_mutex_t *all_mutexes = NULL; // Global list of mutexes

// Mutex initialization
void green_mutex_init(green_mutex_t *mutex) {
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->waiting_list = NULL;
    mutex->next = all_mutexes; // Add to global list
    all_mutexes = mutex;
}


// Mutex lock
void green_mutex_lock(green_mutex_t *mutex) {
    // Block all signals to prevent interruptions
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    while (__sync_lock_test_and_set(&(mutex->locked), 1)) {
        // Mutex is locked; put the current thread in the waiting list
        current_thread->state = WAITING;
        current_thread->waiting_for = mutex->owner;
        enqueue_thread(current_thread, &(mutex->waiting_list));
        schedule();
    }
    // Mutex acquired
    mutex->owner = current_thread;
    // Restore previous signal mask
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

// Mutex unlock
void green_mutex_unlock(green_mutex_t *mutex) {

    // Block all signals to prevent interruptions
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    if (mutex->owner != current_thread) {
        fprintf(stderr, "Mutex unlock error: not the owner\n");
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        return;
    }

    mutex->locked = 0;
    mutex->owner = NULL;

    // Wake up the next waiting thread (if any)
    ult_t *next = dequeue_thread(&(mutex->waiting_list));
    if (next) {
        next->state = READY;
        enqueue_thread(next, &thread_queue);
    }
    // Restore previous signal mask
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

// Mutex destroy
void green_mutex_destroy(green_mutex_t *mutex) {
    // Block all signals to prevent interruptions
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    // Check if mutex is still locked
    if (mutex->locked) {
        fprintf(stderr, "Mutex destroy error: mutex is still locked\n");
        sigprocmask(SIG_SETMASK, &old_mask, NULL);
        return;
    }

    // Wake up all waiting threads (if any)
    ult_t *next;
    while ((next = dequeue_thread(&(mutex->waiting_list))) != NULL) {
        next->state = READY;
        next->waiting_for = NULL;  // Clear any dependency on this mutex
        enqueue_thread(next, &thread_queue);
    }

    // Reset mutex state
    mutex->locked = 0;
    mutex->owner = NULL;
    mutex->waiting_list = NULL;

    // Remove the mutex from the global list of mutexes (all_mutexes)
    if (all_mutexes == mutex) {
        all_mutexes = mutex->next;  // If it's the first mutex in the list
    } else {
        // Traverse the list to find the mutex and unlink it
        green_mutex_t *prev = all_mutexes;
        while (prev != NULL && prev->next != mutex) {
            prev = prev->next;
        }
        if (prev != NULL) {
            prev->next = mutex->next;  // Unlink the mutex from the list
        }
    }

    // Restore previous signal mask
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}
