#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "command_line.h"
#define BUFF_SIZE 500 // TODO need to optimize this



int main(int argc, char *argv[]){
    struct addrinfo hints, *result, *rp;
    int candidate_sockets, server_fd, client_fd, got_info, flags;
    char host_name[BUFF_SIZE], port_num[5], client_addr[BUFF_SIZE];
    struct cli *cli_in;
    struct sockaddr_in client_con;
    socklen_t client_con_size;
    
    parse_cli(argc, argv, &cli_in);

    if(cli_in == NULL){
        printf("ERROR: Something went wrong parsing CLI parameters\n");
        exit(EXIT_FAILURE);
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;
    sprintf(port_num, "%d", cli_in->port);
    flags = NI_NUMERICHOST;


    printf("Starting server on port %d...\n", cli_in->port);
    
    // Find a socket candidate
    candidate_sockets = getaddrinfo(NULL, port_num, &hints, &result);

    if (candidate_sockets != 0){
        printf("Failed to discover an address to bind to\n");
        exit(EXIT_FAILURE);
    }

    // Iterate through the linked list candidate sockets
    for(rp = result; rp != NULL; rp = rp->ai_next){
        server_fd = socket(rp->ai_family, rp->ai_socktype, 0);

        if (!server_fd){
            // Couldn't allocate the fd, continue to next option
            continue;
        }

        if(bind(server_fd, rp->ai_addr, rp->ai_addrlen) == 0){
            // Successfully bound to socket
            break;
        }

        close(server_fd);
    }

    if (rp == NULL){
        printf("Failed to bind to a socket!\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) == -1){
        printf("Failed to start listening on bound soket\n");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    got_info = getnameinfo(rp->ai_addr, rp->ai_addrlen, host_name, BUFF_SIZE, NULL, 0, flags);
    if (got_info != 0){
        printf("ERROR: Could not determine human readable conversion of server info due to: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    printf("Successfully initialized server at %s:%s\n", host_name, port_num);


    // Enter main loop and listen for incoming connections
    // Once a connection is formed, parse the URL to figure out what endpoint needs to
    // be called
    while(1){
        // Check if there is a connection
        // For now, accept creates a blocking call until a connection is received
        client_con_size = sizeof(client_con);
        client_fd = accept(server_fd, (struct sockaddr*)&client_con, &client_con_size); // TODO optimize accept so that it's event driven and running in its own thread
        
        if (client_fd == -1){
            printf("ERROR: Could not accept client connection due to: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        got_info = getnameinfo((struct sockaddr*)&client_con, client_con_size, client_addr, BUFF_SIZE, NULL, 0, flags);
        if (got_info != 0){
            printf("ERROR: Could not determine human readable conversion of server info due to: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        printf("Successfully established connection with %s!\n", client_addr);
        break;
            // If there is, service it and send data back to client fd
            // Close client connection and re-enter the main loop
        // Otherwise, loop again
        ;
    }
    

    // Close client connection and re-enter the main loop

    // Exit and clean up
    free(cli_in);
    freeaddrinfo(result);
    close(server_fd);
    return 0;
    
    
    return 0;
}