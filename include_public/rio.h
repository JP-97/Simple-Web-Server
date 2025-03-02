/**
 * @file rio.h
 * @author Josh Poirier
 * @date 9 Mar 2024
 * @brief File containing some helper APIs for reading/writing to sockets.
 *
 * Since reading an writing to TCP sockets comes with several complexities,
 * this file exposes some helpful APIs to make dealing with sockets a little easier.
 * 
 * These APIs are largely inspired by Computer Systems: A Programmer's Perspective
 * 
 */


#ifndef _RIO
#define _RIO

#include <stddef.h>
#include <sys/types.h>
#define RIO_BUFFSIZE 8192
#define min(a,b) ((a) < (b) ? (a) : (b))

typedef struct rio_struct *rio_t;


/**
 * @brief Initialize handler for descriptor fd.
 * 
 * @param fd File descriptor to be initialized for buffered reads.
 * @return initialized rio_t instance.
 * 
 */
rio_t readn_b_init(int fd);


/**
 * @brief Destroy the provided rio_t instance.
 * 
 * @param instance pointer to rio_t instance to destroy. 
 */
void readn_b_destroy(rio_t *instance);


/**
 * @brief Attempt to read num_bytes from fd into userbuf. 
 * 
 * @param rp Pointer to rio_t struct 
 * @param userbuf Pointer to buffer to read contents into.
 * @param num_bytes Number of Bytes to try and read.
 * @return Number of Bytes read into userbuf, 0 for EOF and -1 for error.
 */
ssize_t readn_b(rio_t rp, char *userbuf, size_t num_bytes);


/**
 * @brief Attempt to read one line from fd into userbuf.
 * 
 * @param rp Pointer to rio_t struct
 * @param userbuf Pointer to buffer to read contents into.
 * @param maxlen Max length of line to read. Longer will be truncated.
 * @return Number of Bytes read into userbuf. 0 for EOF and -1 for error.
 */
ssize_t readline_b(rio_t rp, void *userbuf, size_t maxlen);


/**
 * @brief Attempt to write num_bytes from a userbuf to fd.
 * 
 * @param fd File descriptor to write to.
 * @param userbuf Pointer to buffer containing contents to write.
 * @param num_bytes Number of bytes to write from userbuf.
 * @return 0 if everything was written, otherwise -1.
 */
ssize_t writen_b(int fd, void *userbuf, size_t num_bytes);


/**
 * @brief Attempt to write num_bytes from in_fd to out_fd.
 * 
 * @param out_fd File descriptor to write to.
 * @param in_fd File descriptor to read from.
 * @param num_bytes Number of bytes to write between fds.
 * @return 0 if everything was written, otherwise -1.
 */
ssize_t writen(int out_fd, int in_fd, size_t num_bytes);

#endif