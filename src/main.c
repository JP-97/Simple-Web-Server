#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <semaphore.h>
#include <pthread.h>
#include "command_line.h"
#include "rio.h"
#include "http.h"
#include "bbuf.h"

#define BUFF_SIZE 100 // TODO need to optimize this
#define MAX_IN_LEN 100
#define MAX_BBUFF_LEN 25
#define MAX_SERVER_HOSTNAME_LEN 25
#define NUM_WORKER_THREADS 5

static void run_server(struct cli *cli_in);
static void set_up_signal_handlers();
static int bind_server_port(int server_port, int *svr_fd, char *server_ip);
void *process_incoming_request(void *args);


/**
 * @brief Spawn the server's main loop using provided CLI inputs.
 * 
 * @param cli_in struct containing CLI inputs which will be used to start the server.
 * @return 0 for success (user terminated server), -1 for error starting server.
 */
static void run_server(struct cli *cli_in){
    int client_fd, server_fd, got_info;
    // TODO Clean all these array declarations up so GET methods return a reference instead of needing 2 copies...
    char host_name[MAX_SERVER_HOSTNAME_LEN], client_addr[BUFF_SIZE];
    struct sockaddr_in client_con;
    socklen_t client_con_size;
    pthread_t tids[NUM_WORKER_THREADS];

    bbuf_t *bbuf;
    if(bbuf_init(&bbuf) != 0){
        printf("ERROR: Failed to set up bounded buffer!\n");
        exit(EXIT_FAILURE);
    }

    for(int i=0; i<NUM_WORKER_THREADS; i++){
        pthread_create(&tids[i], NULL, process_incoming_request, bbuf);
    }

    int bound_server_port = bind_server_port(cli_in->port, &server_fd, host_name);
    
    if(bound_server_port != 0){
        printf("ERROR: Failed to bind server to port %d\n", cli_in->port);
        exit(EXIT_FAILURE);
    }

    printf("Successfully initialized server at %s:%d\n" \
           "Enter CTRL+C to kill the server...\n", host_name, cli_in->port);

    // Enter main loop and listen for incoming connections

    while(1){
        // Main thread is used for listening to connections and adding them to the buffer
        client_con_size = sizeof(client_con);
        client_fd = accept(server_fd, (struct sockaddr*) &client_con, &client_con_size);
        
        if (client_fd == -1){
            printf("ERROR: Could not accept client connection due to: %s\n", strerror(errno));
            exit(1);
        }

        // Get human readable conversion of client connection info
        got_info = getnameinfo((struct sockaddr*)&client_con, client_con_size, client_addr, BUFF_SIZE, NULL, 0, NI_NUMERICHOST);
        if (got_info != 0){
            printf("ERROR: Could not determine human readable conversion of client info due to: %s\n", strerror(errno));
            exit(1);
        }

        printf("Successfully established connection with %s!\n", client_addr);
        
        if (bbuf_insert(bbuf, client_fd) != 0){
            printf("WARNING: Failed to insert client connection into buffer...\n");
            continue;
        };
    }

    for(int i=0; i<NUM_WORKER_THREADS; i++){
        pthread_join(tids[i], NULL); // For now, I don't this this code is ever hit... we only exit on error or signal handler.
    }
}


/**
 * @brief Bind and listen on the provided port
 * 
 * @param server_port desired port number for the server.
 * @param svr_fd pointer to the int where the server fd will be stored.
 * @param server_ip pointer to hold the ip addr of the server.
 *
 * @return int 0 if server port was successfully bound, otherwise -1.
 */
static int bind_server_port(int server_port, int *svr_fd, char *server_ip){
    struct addrinfo hints, *result, *rp;
    int candidate_sockets, server_fd, flags;
    char port_num[5];

    // Set up hints struct which will define what type of
    // socket we get allocated
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; 
    hints.ai_flags = AI_PASSIVE;
    flags = NI_NUMERICHOST;

    sprintf(port_num, "%d", server_port);

    // Find a socket candidate
    candidate_sockets = getaddrinfo(NULL, port_num, &hints, &result);

    if (candidate_sockets != 0){
        printf("Failed to discover an address to bind to\n");
        return -1;
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

    if (!rp){
        printf("Failed to bind to a socket!\n");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1){
        printf("Failed to start listening on bound soket\n");
        close(server_fd);
        return -1;
    }

    *svr_fd = server_fd;

    // Get human-readable conversion of server address
    int got_info = getnameinfo(rp->ai_addr, rp->ai_addrlen, server_ip, MAX_SERVER_HOSTNAME_LEN, NULL, 0, flags);
    if (got_info != 0){
        printf("ERROR: Could not determine human readable conversion of server info due to: %s\n", strerror(errno));
        close(server_fd);
        return -1;
    }

    return 0;
}


/**
 * @brief callback for handling incoming client requests
 * 
 * @param args input data for the callback
 * @return void* NULL or exits.
 */
void *process_incoming_request(void *args){
    bbuf_t *buff = (bbuf_t *) args;
    int client_fd;
    // TODO Clean all these array declarations up so GET methods return a reference instead of needing 2 copies...
    char method[MAX_METHOD_LEN], version[MAX_VER_LEN], uri[MAX_URI_LEN], status[MAX_RESP_STATUS_LEN], resp_headers[MAX_RESP_HEADERS_LEN], body[MAX_RESP_BODY_LEN];
    http_req request;
    http_resp response;


    while(1){
        bbuf_remove(buff, &client_fd);
        printf("Processing client request fd %d", client_fd);
        
        init_http_request(client_fd, MAX_IN_LEN, &request);
        init_http_response(&response);

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
        close(client_fd);
    }

    return NULL;
}


static void sig_int_handler(int exit_code){
    printf("\nGoodbye...\n");
    exit(EXIT_SUCCESS);
}


static void sig_term_handler(int exit_code){
    printf("\nweb server was terminated!\n");
    exit(EXIT_FAILURE);
}


static void set_up_signal_handlers(){
    signal(SIGTERM, sig_term_handler);
    signal(SIGINT, sig_int_handler);
}


int main(int argc, char *argv[]){
    struct cli *cli_in;
    parse_cli(argc, argv, &cli_in);

    if(cli_in == NULL){
       return EXIT_FAILURE;
    }

    set_up_signal_handlers();
    run_server(cli_in);
    free(cli_in);
    
    return EXIT_SUCCESS;
}