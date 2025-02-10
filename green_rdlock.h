#ifndef GREEN_RDLOCK_H
#define GREEN_RDLOCK_H

// Read/Write Lock structure
typedef struct green_rwlock{
    int readers;
    int writer;
    ult_t *waiting_writers;
    ult_t *waiting_readers;
    ult_t *held_by; // Pointer to the thread holding the writelock
    struct green_rwlock *next;
} green_rwlock_t;

extern green_rwlock_t *all_rwlocks;
void green_rwlock_init(green_rwlock_t *rwlock);
void green_rwlock_destroy(green_rwlock_t *rwlock);
void green_rwlock_rdlock(green_rwlock_t *rwlock);
void green_rwlock_wrlock(green_rwlock_t *rwlock);
void green_rwlock_unlock(green_rwlock_t *rwlock);


#endif //GREEN_RDLOCK_H
