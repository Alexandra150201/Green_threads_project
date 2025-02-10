#ifndef GREEN_THREADS_H
#define GREEN_THREADS_H

#define STACK_SIZE 1024 * 64
#define QUOTA 50000 // Time slice for round-robin scheduling in microseconds

typedef enum {
    READY,
    RUNNING,
    WAITING,
    TERMINATED
} thread_state;

typedef struct ult {
    ucontext_t context;
    thread_state state;
    void (*function)(void *);
    void *arg;
    struct ult *waiting_for;
    struct ult *next;
    char stack[STACK_SIZE];
} ult_t;

extern ult_t *thread_queue;
extern ult_t *current_thread;

// Functions
void ult_init();
int ult_create(ult_t **thread, void (*function)(void*), void *arg);
void ult_join(ult_t *thread);
void enqueue_thread(ult_t *thread, ult_t **queue);
ult_t* dequeue_thread(ult_t **queue);
void schedule();


#endif // GREEN_THREADS_H