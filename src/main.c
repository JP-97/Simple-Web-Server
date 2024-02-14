#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#define BUFF_SIZE 500

int server_init(void);


int main(void){
    // Start up the server on specified port
    // Bind/Listen on socket

    printf("Starting server...\n");
    server_init();


    // Enter main loop and listen for incoming connections
    // Once a connection is formed, parse the URL to figure out what endpoint needs to
    // be called
    

    // Service that endpoint and send data back to client fd


    // Close client connection and re-enter the main loop
    
    
    
    return 0;
}


int server_init(void){
    struct addrinfo hints, *result, *rp;
    int candidate_sockets, fd, got_info, flags;
    char host_name[BUFF_SIZE], *port_num;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;
    port_num = "8000";
    flags = NI_NUMERICHOST;

    // Find a socket to bind to
    candidate_sockets = getaddrinfo(NULL, port_num, &hints, &result);

    if (candidate_sockets != 0){
        printf("Failed to discover an address to bind to\n");
        exit(EXIT_FAILURE);
    }

    // Iterate through the linked list candidate sockets and find one
    // we can bind to
    for(rp = result; rp != NULL; rp = rp->ai_next){
        fd = socket(rp->ai_family, rp->ai_socktype, 0);

        if (!fd){
            // Couldn't allocate the fd, continue to next option
            continue;
        }

        if(bind(fd, rp->ai_addr, rp->ai_addrlen) == 0){
            // Successfully bound to socket
            break;
        }

        close(fd);
    }

    if (rp == NULL){
        printf("Failed to bind to a socket!\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 10) == -1){
        printf("Failed to start listening on bound soket\n");
        close(fd);
        exit(EXIT_FAILURE);
    }

    got_info = getnameinfo(rp->ai_addr, rp->ai_addrlen, host_name, BUFF_SIZE, NULL, 0, flags);
    printf("Successfully initialized server at %s:%s\n", host_name, port_num);
    
    // Run server indefinitely
    while(1){;}

    //TODO: Right now this is basically dead code, since the only way to get out of infinite loop is killing the program...
    freeaddrinfo(result);
    close(fd);
    return 0;
}