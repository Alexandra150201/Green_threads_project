#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "green_threads.h"
#include "green_rdlock.h"

// Signal mask to block all signals
static sigset_t block_all_signals;
green_rwlock_t *all_rwlocks = NULL;

// Read/Write Lock initialization
void green_rwlock_init(green_rwlock_t *rwlock) {
    rwlock->readers = 0;
    rwlock->writer = 0;
    rwlock->waiting_writers = NULL;
    rwlock->waiting_writers = NULL;
    rwlock->held_by = NULL;
    rwlock->next = all_rwlocks; // Add to global list
    all_rwlocks = rwlock;
}

// Read/Write Lock destroy
void green_rwlock_destroy(green_rwlock_t *rwlock) {
    // Block all signals to prevent interruptions
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    if (rwlock->writer || rwlock->readers > 0) {
        fprintf(stderr, "RWLock destroy error: lock is still in use\n");
        return;
    }

    rwlock->readers = 0;
    rwlock->writer = 0;
    rwlock->waiting_writers = NULL;
    rwlock->waiting_readers = NULL;

    // Restore previous signal mask
    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

// Acquire read lock
void green_rwlock_rdlock(green_rwlock_t *rwlock) {
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    while (rwlock->writer) {
        current_thread->state = WAITING;
        current_thread->waiting_for = rwlock->held_by; // Track which thread it is waiting for
        enqueue_thread(current_thread, &(rwlock->waiting_readers));
        schedule();
    }
    rwlock->readers++;

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

// Acquire write lock
void green_rwlock_wrlock(green_rwlock_t *rwlock) {
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    while (rwlock->writer || rwlock->readers > 0) {
        current_thread->state = WAITING;
        current_thread->waiting_for = rwlock->held_by; // Track which thread it is waiting for
        enqueue_thread(current_thread, &(rwlock->waiting_writers));
        schedule();
    }
    rwlock->writer = 1;
    rwlock->held_by = current_thread; // Track which thread owns thewrite lock

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

// Unlock Read/Write Lock
void green_rwlock_unlock(green_rwlock_t *rwlock) {
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    if (rwlock->writer) {
        // Ensure the thread trying to unlock is the one that holds the write lock
        if (rwlock->held_by != current_thread) {
            fprintf(stderr, "RWLock unlock error: thread %p does not own the lock\n", current_thread);
            sigprocmask(SIG_SETMASK, &old_mask, NULL);
            return;
        }

        rwlock->writer = 0;
        rwlock->held_by = NULL;  // Reset ownership
    } else if (rwlock->readers > 0) {
        rwlock->readers--;
    }

    if (rwlock->readers == 0 && rwlock->waiting_writers) {
        ult_t *next = dequeue_thread(&(rwlock->waiting_writers));
        if (next) {
            next->state = READY;
            enqueue_thread(next, &thread_queue);
        }
    } else if (rwlock->readers == 0 && rwlock->waiting_readers) {
        ult_t *next;
        while ((next = dequeue_thread(&(rwlock->waiting_readers))) != NULL) {
            next->state = READY;
            rwlock->readers++;  // Increase reader count
            enqueue_thread(next, &thread_queue);
        }
    }

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}
