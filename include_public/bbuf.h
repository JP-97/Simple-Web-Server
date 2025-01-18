#ifndef _BBUF
#define _BBUF

#include <semaphore.h>

typedef struct _bbuf *bbuf_t;

/**
 * @brief Initialize an empty bounded buffer
 * 
 * @return a handle to the buffer.
 */
bbuf_t bbuf_init();


/**
 * @brief Free all memory allocated to bounded buffer
 * 
 * @param bbuf_to_destroy bbuf instance to be destroyed
 */
void bbuf_destroy(bbuf_t bbuf_to_destroy);

/**
 * @brief Add an item to the bounded buffer. This will block until
 * There's an available slot in the buffer and the mutex can be acquired.
 * 
 * @param bbuf reference to the bbuf_t handler you want to insert into.
 * @param fd_to_insert int containing the file descriptor to insert into the buffer.
 * @return int 0 on success of adding fd, otherwise -1.
 */
int bbuf_insert(bbuf_t bbuf, int fd_to_insert);

/**
 * @brief Remove an item from the bounded buffer. This will block until
 * There's an item to remove from the buffer and the mutex can be acquired.
 * 
 * @param bbuf reference to the bbuf_t handler you want to removed from.
 * @param fd pointer to int that holds the removed file descriptor. NULL to ignore.
 * @return int 0 on success, otherwise -1.
 */
int bbuf_remove(bbuf_t bbuf, int *fd);

/**
 * @brief Get size of the buffer
 * 
 * @param bbuf reference to the bbuf_t you want to get size of.
 * @return int max number of elements in the buffer, -1 on uninitialized buffer.
 */
int get_bbuf_max_size(bbuf_t bbuf);

/**
 * @brief Return number of free slots in the buffer
 * 
 * @param bbuf reference to the bbuf_t you want to get available slots from
 * @return int number of free slots, -1 on uninitialized buffer.
 */
int get_bbuf_slots(bbuf_t bbuf);

/**
 * @brief Return number of items stored in the buffer
 * 
 * @param bbuf reference to the bbuf_t you want to get number if items from
 * @return int number of items in the buffer, -1 on uninitialized buffer.
 */
int get_bbuf_items(bbuf_t bbuf);

#endif