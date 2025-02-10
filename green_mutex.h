#ifndef GREEN_MUTEX_H
#define GREEN_MUTEX_H

typedef struct green_mutex {
    volatile int locked;
    ult_t *owner;
    ult_t *waiting_list;
    struct green_mutex *next;
} green_mutex_t;

extern green_mutex_t *all_mutexes;

void green_mutex_init(green_mutex_t *mutex);
void green_mutex_lock(green_mutex_t *mutex);
void green_mutex_unlock(green_mutex_t *mutex);
void green_mutex_destroy(green_mutex_t *mutex);

#endif //GREEN_MUTEX_H
