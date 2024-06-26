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
#include "rio.h"
#include "http.h"
#define BUFF_SIZE 100 // TODO need to optimize this
#define MAX_IN_LEN 100

static int run_server(struct cli *cli_in);
static void user_exit_handler(int exit_code);


int main(int argc, char *argv[]){
    int was_started;
    struct cli *cli_in;
    parse_cli(argc, argv, &cli_in);

    if(cli_in == NULL){
        exit(EXIT_FAILURE);
    }

    was_started = run_server(cli_in);
    free(cli_in);
    
    if (was_started == -1){
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
    int candidate_sockets, server_fd, client_fd, got_info, flags, bytes_read;
    // TODO Clean all these array declarations up so GET methods return a reference instead of needing 2 copies...
    char host_name[BUFF_SIZE], port_num[5], client_addr[BUFF_SIZE], in_buf[BUFF_SIZE], method[MAX_METHOD_LEN], version[MAX_VER_LEN], uri[MAX_URI_LEN], status[MAX_RESP_STATUS_LEN], resp_headers[MAX_RESP_HEADERS_LEN], body[MAX_RESP_BODY_LEN];
    struct sockaddr_in client_con;
    socklen_t client_con_size;
    http_req request;
    http_resp response;

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


    if (signal(SIGINT, user_exit_handler)){
        freeaddrinfo(result);
        close(server_fd);
        return 0;
    }

    // Enter main loop and listen for incoming connections
    // Once a connection is formed, parse the URL to figure out what endpoint needs to
    // be called

    while(1){
        // Check if there is a connection
        // For now, accept creates a blocking call until a connection is received
        client_con_size = sizeof(client_con);
        client_fd = accept(server_fd, (struct sockaddr*) &client_con, &client_con_size); // TODO optimize accept so that it's event driven and running in its own thread
        
        if (client_fd == -1){
            printf("ERROR: Could not accept client connection due to: %s\n", strerror(errno));
            exit(1);
        }

        // Get human readable conversion of client connection info
        got_info = getnameinfo((struct sockaddr*)&client_con, client_con_size, client_addr, BUFF_SIZE, NULL, 0, flags);
        if (got_info != 0){
            printf("ERROR: Could not determine human readable conversion of client info due to: %s\n", strerror(errno));
            exit(1);
        }


        // TODO All of client connection handling should be done in a separate process
        printf("Successfully established connection with %s!\n", client_addr);
        init_http_request(client_fd, MAX_IN_LEN, &request);
        init_http_response(&response);

        printf("Processing HTTP request...\n");    
        get_http_request_method(request, method);
        get_http_request_uri(request, uri);
        get_http_request_version(request, version);

        printf("Got the following request:\n \
        METHOD:   %s\n \
        URI:      %s\n \
        HTTP VER: %s\n", method, uri, version);

        printf("Formulating HTTP response...\n");
        int result = get_http_response_from_request(request, response);

        if (result == -1){
            printf("ERROR: Something went wrong processing HTTP request\n");
            exit(1);
        }

        get_http_response_status(response, status, MAX_RESP_STATUS_LEN);
        get_http_response_headers(response, resp_headers, MAX_RESP_HEADERS_LEN);
        get_http_response_body(response, body, MAX_RESP_BODY_LEN);

        printf("Formulated the following response:\n \
        STATUS:  %s\n  \
        HEADERS: %s\n  \
        BODY:    %s    \n", status, resp_headers, body);

        printf("Sending the HTTP response back to the client...\n");
        int bytes_written = writen(client_fd, status, strlen(status));
        if(bytes_written == -1){
            exit(1);
        }

        bytes_written = writen(client_fd, resp_headers, strlen(resp_headers));
        if(bytes_written == -1){
            exit(1);
        }

        bytes_written = writen(client_fd, body, strlen(body));
        if(bytes_written == -1){
            exit(1);
        }
        
        destroy_http_response(&response);
        destroy_http_request(&request);
        // exit(0);
    }
}


static void user_exit_handler(int exit_code){
    printf("\nGoodbye...\n");
    exit(EXIT_SUCCESS);
}