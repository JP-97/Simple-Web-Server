#include <semaphore.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include "bbuf_private.h"
#include "log.h"

/*Forward Declarations*/
static sem_t *init_unamed_semaphore(unsigned int initial_value);


bbuf_t bbuf_init(){
    bbuf_t tmp_buff = (bbuf_t) calloc(1, sizeof(struct _bbuf));

    if(!tmp_buff){
        perror("ERROR: Failed to initialize bounded buffer\n");
        return NULL;
    }

    sem_t *mutex = init_unamed_semaphore(1);
    if(!mutex){
        perror("ERROR: Failed to initialize bounded buffer mutex\n");
        return NULL;
    }

    sem_t *slots = init_unamed_semaphore(25);
    if(!slots){
        perror("ERROR: Failed to initialize bounded buffer slots semaphore\n");
        return NULL;
    }

    sem_t *items = init_unamed_semaphore(0);
    if(!items){
        perror("ERROR: Failed to initialize bounded buffer items semaphore\n");
        return NULL;
    }

    tmp_buff->max_size = BBUF_SIZE;
    tmp_buff->front = 0;
    tmp_buff->rear = 0;
    
    tmp_buff->mutex = mutex;
    tmp_buff->slots = slots;
    tmp_buff->items = items;

    return tmp_buff;
}

void bbuf_destroy(bbuf_t bbuf){    
    free(bbuf->mutex);
    free(bbuf->slots);
    free(bbuf->items);
    free(bbuf);
    return;
}

int bbuf_insert(bbuf_t bbuf, int fd_to_insert){
    if(!bbuf){
        LOG(ERROR, "NULL buffer reference provided!\n");
        return -1;
    }
    pthread_t thread_id = pthread_self();
    // Wait until there's a free slot
    sem_wait(bbuf->slots);
    // Wait until the mutex is free
    sem_wait(bbuf->mutex);
    // Insert slot at end of buffer
    bbuf->buff[(bbuf->rear)%(bbuf->max_size)] = fd_to_insert;
    (bbuf->rear)++;
    LOG(DEBUG, "Thread %lu: Inserted fd %d at index %d\n", (unsigned long)thread_id, fd_to_insert, (bbuf->rear)%(bbuf->max_size));
    // Release mutex
    sem_post(bbuf->mutex);
    // Increment Item
    sem_post(bbuf->items);
    return 0;
}

int bbuf_remove(bbuf_t bbuf, int *fd){
    int removed_fd;

    if(!bbuf){
        LOG(ERROR,"ERROR: NULL buffer reference provided!\n");
        return -1;
    }

    pthread_t thread_id = pthread_self();
    // Wait until there's an item
    sem_wait(bbuf->items);
    // Wait until the mutex is free
    sem_wait(bbuf->mutex);
    // Remove item from front of buffer
    removed_fd = bbuf->buff[(bbuf->front)%(bbuf->max_size)];
    (bbuf->front)++;
    LOG(DEBUG, "Thread %lu: Removed fd %d from index %d\n",(unsigned long)thread_id, removed_fd, (bbuf->front)%(bbuf->max_size));
    // Release mutex
    sem_post(bbuf->mutex);
    // Increment slot
    sem_post(bbuf->slots);

    if(fd) *fd = removed_fd;
    
    return 0;
}

int get_bbuf_max_size(bbuf_t bbuf){
    if(!bbuf){
        return -1;
    }
    return bbuf->max_size;
}

int get_bbuf_slots(bbuf_t bbuf){
    int tmp, rc;
    if(!bbuf){
        LOG(ERROR, "provided handle is null\n");
        return -1;
    }
    rc = sem_getvalue(bbuf->slots, &tmp);
    if(rc != 0){
        LOG(ERROR, "Failed to get number of free slots\n");
        return rc;
    }
    return tmp;
}

int get_bbuf_items(bbuf_t bbuf){
    int tmp, rc;
    if(!bbuf){
        LOG(ERROR, "provided handle is null\n");
        return -1;
    }
    rc = sem_getvalue(bbuf->items, &tmp);
    if(rc != 0){
        LOG(ERROR, "Failed to get number of free items\n");
        return rc;
    }
    return tmp;
}

static sem_t *init_unamed_semaphore(unsigned int initial_value){
    sem_t *sem = (sem_t *) malloc(sizeof(sem_t));
    if(!sem){
        return NULL;
    }

    sem_init(sem, 0, initial_value);
    return sem;
}