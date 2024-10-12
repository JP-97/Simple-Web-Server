#ifndef _BBUF
#define _BBUF

#include <semaphore.h>

#define BBUF_SIZE 25

typedef struct{
    int front;   // buff[(front + 1) % max_size]
    int rear;    // buff[rear % max_size]
    int max_size;
    int *buff;   // Pointer to bounded buffer which will contain the file descriptors
    sem_t *mutex; // Synchronize access to buff
    sem_t *slots; // Number of free slots in buff
    sem_t *items; // Number of items stored in buff
} bbuf_t; //bbuf_t is a point to a _bbuf struct

/**
 * @brief Initialize an empty bounded buffer
 * 
 * @param bbuf pointer-to-pointer of bounded buffer
 * @return int 0 on success, otherwise -1
 */
int bbuf_init(bbuf_t **bbuf);

/**
 * @brief Free bounded buffer
 * 
 * @param bbuf pointer to bounded buffer to be freed
 * @return int 0 on success, otherwise -1
 */
int bbuff_free(bbuf_t **bbuf);

/**
 * @brief Add an item to the bounded buffer. This will block until
 * There's an available slot in the buffer and the mutex can be acquired.
 * 
 * @param bbuf pointer to buff to insert file descriptor.
 * @param fd_to_insert int containing the file descriptor to insert into the buffer.
 * @return int 0 on success of adding fd, otherwise -1.
 */
int bbuf_insert(bbuf_t *bbuf, int fd_to_insert);

/**
 * @brief Remove an item from the bounded buffer. This will block until
 * There's an item to remove from the buffer and the mutex can be acquired.
 * 
 * @param bbuf pointer to buff to remove file descriptor from.
 * @param fd pointer to int that holds the removed file descriptor.
 * @return int 0 on success, otherwise -1.
 */
int bbuf_remove(bbuf_t *bbuf, int *fd);

/**
 * @brief Get size of the buffer
 * 
 * @param bbuf pointer to the buffer
 * @return int number of elements in the buffer, -1 on uninitialized buffer.
 */
int get_bbuf_size(bbuf_t *bbuf);

/**
 * @brief Return number of free slots in the buffer
 * 
 * @param bbuf pointer to the buffer 
 * @return int number of free slots, -1 on uninitialized buffer.
 */
int get_bbuf_slots(bbuf_t *bbuf);

/**
 * @brief Return number of items stored in the buffer
 * 
 * @param bbuf reference to the buffer
 * @return int number of items in the buffer, -1 on uninitialized buffer.
 */
int get_bbuf_items(bbuf_t *bbuf);

#endif