#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/sendfile.h>
#include "rio.h"

#define EXIT_FAILURE_RIO -1


// Intenal helper used amongst public APIs
static ssize_t read_b(rio_t rp, char *userbuf, size_t num_to_read);


/**
 * This struct is for internal use only.
 */
struct rio_struct {
    int fd;                     /**< File descriptor to read from. */
    ssize_t remaining;          /**< Remanining number of bytes in internal buffer. */
    char *rio_bufptr;           /**< Pointer to next byte to be read from internal buffer. */
    char rio_buf[RIO_BUFFSIZE]; /**< Internal buffer. */
    ssize_t total_bytes_copied; /**< Total number of bytes copied from internal buffer to user buffer */
};


rio_t readn_b_init(int fd){
    rio_t resp = (rio_t) calloc(1, sizeof(struct rio_struct));

    if(!resp) return NULL;

    resp->fd = fd;
    resp->remaining = 0;
    resp->rio_bufptr = resp->rio_buf;
    resp->total_bytes_copied = 0;
    
    return resp;
}


void readn_b_destroy(rio_t *instance){
    // Return if already NULL
    if (!instance) return;
    
    free(*instance);
    *instance = NULL;

}


ssize_t readn_b(rio_t rp, char *userbuf, size_t num_bytes){
    ssize_t bytes_copied;
    size_t bytes_left_to_copy = num_bytes;
    rp->total_bytes_copied = 0;

    while (bytes_left_to_copy > 0){
        
        bytes_copied = read_b(rp, userbuf, bytes_left_to_copy);

        if (bytes_copied == 0){
            break;
        
        } else if (bytes_copied == -1){
            printf("ERROR: Failed trying to read %ld bytes from fd %d...\n", num_bytes, rp->fd);
            return -1;
        }

        bytes_left_to_copy -= bytes_copied;
        rp->total_bytes_copied += bytes_copied;
    }

    return num_bytes - bytes_left_to_copy;
}


ssize_t readline_b(rio_t rp, void *userbuf, size_t maxlen){
    
    char *ptr = (char *)userbuf;
    int i = 0, num_read = 0, was_read;

    while (num_read < maxlen){

        // Read one character at a time until new line is encountered
        was_read = read_b(rp, ptr, 1);

        if (was_read == 0 && num_read == 0){
            // EOF - nothing was read
            return 0;

        } else if (was_read == 0){
            // EOF - some data was read
            break;

        } else if (was_read == -1){
            printf("ERROR: Failed trying to read a byte from %d...\n", rp->fd);
            return -1;
        
        } else if (((char*)userbuf)[i] == '\n'){
            // End of line encountered
            num_read++;
            printf("Found end of line after reading %d bytes\n", num_read);
            return num_read;
        }
        i++;
        ptr++;
        num_read++;
       
    }
    return num_read;
}


ssize_t writen_b(int fd, void *userbuf, size_t num_bytes){
    int max_retries = 5, retry_num = 0, bytes_left = num_bytes;
    ssize_t bytes_written = 0;
    char *bufp = (char *)userbuf;
    
    // Retry the write until num_bytes has been transfered to fd
    // or we encounter write sys call explicitly fails
    while (bytes_left > 0 && retry_num < max_retries){
        bytes_written = write(fd, bufp, num_bytes);
        
        if (bytes_written == -1){
            printf("ERROR: Failed to write %ldB to fd %d...\n", num_bytes, fd);
            return EXIT_FAILURE_RIO;
        }

        else if (errno == EINTR){
            printf("Encountered signal %d, retrying...\n", errno);
            bytes_written = 0;
            bufp = userbuf;
            retry_num++;
            continue;
        }

        bytes_left -= bytes_written;
        bufp += bytes_written;
    }

    printf("Successfully wrote %ldB to fd %d...\n", bytes_written, fd);
    return EXIT_SUCCESS;
}


ssize_t writen(int out_fd, int in_fd, size_t num_bytes){
    int max_retries = 5;
    int retry_num = 0;
    size_t bytes_left = num_bytes;
    ssize_t bytes_written = 0;
    
    // Retry the write until num_bytes has been transfered to fd
    // or we encounter write sys call explicitly fails
    while (bytes_left > 0 && retry_num < max_retries){
        bytes_written = sendfile(out_fd, in_fd, NULL, num_bytes);

        if (bytes_written == -1){

            if (errno == EINTR){
                printf("Encountered signal %d, retrying...\n", errno);
                bytes_written = 0;
                retry_num++;
                continue;
            }

            printf("ERROR: Failed to write %ldB to fd %d...\n", num_bytes, out_fd);
            return EXIT_FAILURE_RIO;
        }

        bytes_left -= bytes_written;
    }

    printf("Successfully wrote %ldB to fd %d...\n", bytes_written, out_fd);
    return EXIT_SUCCESS;
}


/**
 * This function is just a buffered version of read(). It reads max
 * amount of data into internal rio_buf and then copies required amount
 * to userbuf. It can act as a direct substitute for standard read and
 * will save on number of system calls.
 */
static ssize_t read_b(rio_t rp, char *userbuf, size_t num_to_read){
    size_t num_copied;

    // Refill the rio_buf if it's empty
    while(rp->remaining <= 0){
        rp->rio_bufptr = rp->rio_buf; // reset buf ptr
    
        rp->remaining = read(rp->fd, rp->rio_buf, sizeof(rp->rio_buf));
        if (rp->remaining == -1 && errno != EINTR){
            printf("ERROR: Encountered the following error trying to read from fd %d: %s\n", rp->fd, strerror(errno));
            return -1;
        
        } else if (rp->remaining == 0){
            return 0; // EOF
        }
    }

    // Copy data from rio_buf to userbuf
    num_copied = min(num_to_read, rp->remaining); // It's likely we were able to copy more to internal buf than required
    memcpy(userbuf + rp->total_bytes_copied, rp->rio_bufptr, num_copied);
    rp->remaining -= num_copied;
    rp->rio_bufptr += num_copied;

    return num_copied;

}
