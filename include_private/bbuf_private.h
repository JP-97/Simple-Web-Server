#ifndef _BBUF_PRIVATE
#define _BBUF_PRIVATE

#include "bbuf.h"

#define BBUF_SIZE 25

struct _bbuf {
    int front;   // buff[(front + 1) % max_size]
    int rear;    // buff[rear % max_size]
    int max_size;
    int *buff;   // Pointer to bounded buffer which will contain the file descriptors
    sem_t *mutex; // Synchronize access to buff
    sem_t *slots; // Number of free slots in buff
    sem_t *items; // Number of items stored in buff
};

#endif