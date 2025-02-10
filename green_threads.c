#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>
#include "green_threads.h"

// Global variables
ult_t *current_thread = NULL;
ult_t *thread_queue = NULL;
// Signal mask to block all signals
static sigset_t block_all_signals;

// Function declarations
static void thread_wrapper();

// Initialize the main thread
void ult_init() {
    current_thread = (ult_t*)malloc(sizeof(ult_t));
    if (current_thread == NULL) {
        perror("Failed to allocate memory for main thread");
        exit(EXIT_FAILURE);
    }

    getcontext(&(current_thread->context));
    current_thread->state = RUNNING;
    current_thread->next = NULL;
    thread_queue = current_thread;

    // Setup preemption
    struct sigaction sa;
    sa.sa_handler = schedule;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NODEFER;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 0;
    timer.it_value.tv_usec = QUOTA;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = QUOTA;
    setitimer(ITIMER_REAL, &timer, NULL);
}

// Create a new thread
int ult_create(ult_t **thread, void (*function)(void*), void *arg) {
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);
    *thread = (ult_t*)malloc(sizeof(ult_t));
    if (*thread == NULL) {
        perror("Failed to allocate memory for thread");
        exit(EXIT_FAILURE);
    }

    ult_t *new_thread = *thread;
    getcontext(&(new_thread->context));
    new_thread->function = function;
    new_thread->arg = arg;
    new_thread->state = READY;
    new_thread->waiting_for = NULL;

    new_thread->context.uc_link = NULL;
    new_thread->context.uc_stack.ss_sp = new_thread->stack;
    new_thread->context.uc_stack.ss_size = STACK_SIZE;
	
    makecontext(&(new_thread->context), (void(*)())thread_wrapper, 0);
    enqueue_thread(new_thread, &thread_queue);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
    return 0;
}

// Wrapper to execute thread functions
static void thread_wrapper() {
    ult_t *thread = current_thread;
    thread->function(thread->arg);
    thread->state = TERMINATED;
    printf("Thread %p terminated\n", thread);
    schedule();
}

// Join a thread
void ult_join(ult_t *thread) {
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);
    while (thread->state != TERMINATED) {
        current_thread->state = WAITING;
        current_thread->waiting_for = thread;
        schedule();
    }
    current_thread->waiting_for = NULL;
    current_thread->state = READY;
    enqueue_thread(current_thread, &thread_queue);

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}
//round robin
void schedule() {
    sigset_t old_mask;
    sigprocmask(SIG_BLOCK, &block_all_signals, &old_mask);

    ult_t *previous = current_thread;

    if (current_thread == NULL) {
        fprintf(stderr, "Error: current_thread is NULL\n");
        exit(EXIT_FAILURE);
    }

    // Remove current thread from queue if it is WAITING
    if (previous->state == WAITING) {
        remove_thread_from_queue(previous, &thread_queue);
    }

    // Find next thread to run
    ult_t *next_thread = previous->next;
    if (next_thread == NULL) {
        next_thread = thread_queue;
    }

    while (next_thread != NULL &&
           (next_thread->state == TERMINATED ||
            (next_thread->state == WAITING && next_thread->waiting_for != NULL && next_thread->waiting_for->state != TERMINATED))) {
        next_thread = next_thread->next;
        if (next_thread == NULL) {
            next_thread = thread_queue;
        }
    }

    // Switch context if a valid thread is found
    if (next_thread != NULL && previous != next_thread) {
        printf("Switching from thread %p to thread %p\n", previous, next_thread);
        current_thread = next_thread;
        swapcontext(&(previous->context), &(next_thread->context));
    }

    sigprocmask(SIG_SETMASK, &old_mask, NULL);
}

// Helper function to remove a thread from the queue
void remove_thread_from_queue(ult_t *thread, ult_t **queue) {
    if (!*queue) return;

    ult_t *prev = NULL, *curr = *queue;

    while (curr) {
        if (curr == thread) {
            if (prev) {
                prev->next = curr->next;
            } else {
                *queue = curr->next;  // Head removal case
            }
            curr->next = NULL;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}



// Add a thread to the queue
void enqueue_thread(ult_t *thread, ult_t **queue) {
    if (!*queue) { //empty queue
        *queue = thread;
        thread->next = NULL;
    } else {
        ult_t *temp = *queue;
        while (temp->next) { //traverse to the end od the queue
                temp = temp->next;
        }
        temp->next = thread;
        thread->next = NULL;
    }
}

// Remove a thread from the queue
ult_t* dequeue_thread(ult_t **queue) {
    if (!*queue) return NULL;

    ult_t *thread = *queue;
    *queue = thread->next;
    thread->next = NULL;
    return thread;
}
