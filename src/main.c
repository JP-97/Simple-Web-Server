#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "command_line.h"
#define BUFF_SIZE 500 // TODO need to optimize this

static int run_server(struct cli *cli_in);
static void user_exit_handler(int exit_code);




int main(int argc, char *argv[]){
    int was_started;
    struct cli *cli_in;
    parse_cli(argc, argv, &cli_in);

    if(cli_in == NULL){
        printf("ERROR: Something went wrong parsing CLI parameters\n");
        exit(EXIT_FAILURE);
    }

    was_started = run_server(cli_in);
    free(cli_in);
    
    if (was_started == -1){
        printf("ERROR: Something went wrong starting server\n");
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);

}


/**
 * @brief Spawn the server's main loop using provided CLI inputs.
 * 
 * @param cli_in struct containing CLI inputs which will be used to start the server.
 * @return 0 for success (user terminated server), -1 for error starting server.
 */
static int run_server(struct cli *cli_in){
    struct addrinfo hints, *result, *rp;
    int candidate_sockets, server_fd, client_fd, got_info, flags;
    char host_name[BUFF_SIZE], port_num[5], client_addr[BUFF_SIZE];
    struct sockaddr_in client_con;
    socklen_t client_con_size;

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
        return -1;
    }

    if (listen(server_fd, 10) == -1){
        printf("Failed to start listening on bound soket\n");
        close(server_fd);
        return -1;
    }

    // Get human-readable conversion of server address
    got_info = getnameinfo(rp->ai_addr, rp->ai_addrlen, host_name, BUFF_SIZE, NULL, 0, flags);
    if (got_info != 0){
        printf("ERROR: Could not determine human readable conversion of server info due to: %s\n", strerror(errno));
        close(server_fd);
        return -1;
    }
    printf("Successfully initialized server at %s:%s\n" \
           "Enter CTRL+C to kill the server...\n", host_name, port_num);


    // Enter main loop and listen for incoming connections
    // Once a connection is formed, parse the URL to figure out what endpoint needs to
    // be called

    if (signal(SIGINT, user_exit_handler)){
        freeaddrinfo(result);
        close(server_fd);
        return 0;
    }

    while(1){
        // Check if there is a connection
        // For now, accept creates a blocking call until a connection is received


        client_con_size = sizeof(client_con);
        client_fd = accept(server_fd, (struct sockaddr*) &client_con, &client_con_size); // TODO optimize accept so that it's event driven and running in its own thread
        
        if (client_fd == -1){
            printf("ERROR: Could not accept client connection due to: %s\n", strerror(errno));
            return -1;
        }

        // Get human readable conversion of client connection info
        got_info = getnameinfo((struct sockaddr*)&client_con, client_con_size, client_addr, BUFF_SIZE, NULL, 0, flags);
        if (got_info != 0){
            printf("ERROR: Could not determine human readable conversion of client info due to: %s\n", strerror(errno));
            return -1;
        }

        printf("Successfully established connection with %s!\n", client_addr);
    }

}

static void user_exit_handler(int exit_code){
    printf("Goodbye...\n");
    return;
}