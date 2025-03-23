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
#include <stdbool.h>
#include "command_line.h"
#include "rio.h"
#include "http.h"
#include "bbuf.h"
#include "log.h"

#define BUFF_SIZE 100 // TODO need to optimize this
#define MAX_BBUFF_LEN 25
#define MAX_SERVER_HOSTNAME_LEN 25
#define NUM_WORKER_THREADS 5

typedef struct _worker_data_t {
    bbuf_t bbuf;
} worker_data_t;

// TODO: Lets create something cool for signal handling where if 
// one of the worker threads runs into a problem a signal is sent up to a monitor thread (main thread?)
// which then terminates the other worker threads gracefully. This will be a lot better than just killing
// the whole process mid request - we can allow the thread to finish the request it's handling then process
// the shutdown signal.

// When user gives CTRL+C (or some other kill) the handler should

//Acknowledge the signal and log a message to the output console of the server.
//Notify all the connected clients that the server is going offline.
//Give the clients enough time (specified by a timeout parameter) to close the requests.
//Close all the client requests and then shut down the server after the timeout exceeds.



static void run_server(struct cli *cli_in);
static void set_up_signal_handlers();
static bool set_up_worker_pool(worker_data_t *worker_data);
static int bind_server_port(int server_port, int *svr_fd, char *server_ip);
static int parse_request(int client_fd, http_req *result);
void *process_incoming_request(void *args);

/**
 * @brief Spawn the server's main loop using provided CLI inputs.
 * 
 * @param cli_in struct containing CLI inputs which will be used to start the server.
 * @return 0 for success (user terminated server), -1 for error starting server.
 */
static void run_server(struct cli *cli_in){
    int client_fd;
    int server_fd;
    int got_info;
    char host_name[MAX_SERVER_HOSTNAME_LEN];
    char client_addr[BUFF_SIZE];
    struct sockaddr_in client_con;
    socklen_t client_con_size;
    worker_data_t worker_data;

    if(!cli_in){
        LOG(ERROR,"Failed in initializing internal data structures!\n");
        return;
    }

    LOG(INFO, "Initializing server with root directory: %s\n", cli_in->server_root);

    // Set up bounded buffer and worker pool
    bbuf_t bbuf = bbuf_init();

    if(!bbuf){
        exit(EXIT_FAILURE);
    }

    // Note: All worker threads are initialized with same
    // bbuf since the buffer is specifically built to operate with
    // multiple users and synchronizes the access internally.
    // worker_data can be stored on the stack because this function
    // survives for the lifeftime of the program.
    worker_data.bbuf = bbuf;

    if(!set_up_worker_pool(&worker_data)){
        exit(EXIT_FAILURE);
    }
    
    // Set up server port
    int bound_server_port = bind_server_port(cli_in->port, &server_fd, host_name);
    
    if(bound_server_port != 0){
        LOG(ERROR,"Failed to bind server to port %d\n", cli_in->port);
        exit(EXIT_FAILURE);
    }

    LOG(INFO,"Successfully initialized server at %s:%d\n" \
             "Enter CTRL+C to kill the server...\n", host_name, cli_in->port);

    // Enter main loop and listen for incoming connections
    while(1){
        client_con_size = sizeof(client_con);
        client_fd = accept(server_fd, (struct sockaddr*) &client_con, &client_con_size);
        
        if (client_fd == -1){
            LOG(ERROR,"Could not accept client connection due to: %s\n", strerror(errno));
            exit(1);
        }

        // Get human readable conversion of client connection info
        got_info = getnameinfo((struct sockaddr*)&client_con, client_con_size, client_addr, BUFF_SIZE, NULL, 0, NI_NUMERICHOST);
        if (got_info != 0){
            LOG(ERROR,"Could not determine human readable conversion of client info due to: %s\n", strerror(errno));
            exit(1);
        }

        LOG(INFO,"Successfully established connection with %s!\n", client_addr);

        // Add the client FD to bounded buffer so it can be handled by one of workers
        if (bbuf_insert(bbuf, client_fd) != 0){
            LOG(ERROR,"WARNING: Failed to insert client connection into buffer...\n");
            continue;
        };
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
        LOG(ERROR,"Failed to discover an address to bind to\n");
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
        LOG(ERROR,"Failed to bind to a socket!\n");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1){
        LOG(ERROR,"Failed to start listening on bound soket\n");
        close(server_fd);
        return -1;
    }

    *svr_fd = server_fd;

    // Get human-readable conversion of server address
    int got_info = getnameinfo(rp->ai_addr, rp->ai_addrlen, server_ip, MAX_SERVER_HOSTNAME_LEN, NULL, 0, flags);
    if (got_info != 0){
        LOG(ERROR,"Could not determine human readable conversion of server info due to: %s\n", strerror(errno));
        close(server_fd);
        return -1;
    }

    return 0;
}


/**
 * @brief callback for handling incoming client requests
 * 
 * @note This function runs indefinitely, meaning it doesn't
 *       exit until the server as a whole is closed. Once a request
 *       is processed, the worker thread re-enters its loop where it monitors
 *       for new client FDs added to the buffer.
 * 
 * @param args input data for the callback
 * @return void* NULL or exits.
 */
void *process_incoming_request(void *args){
    int client_fd;
    int ressource_fd;
    char status[MAX_RESP_STATUS_LEN];
    char resp_headers[MAX_RESP_HEADERS_LEN];
    off_t content_size;
    http_req request;
    http_resp response;
    ssize_t bytes_written;

    pthread_detach(pthread_self());

    if(!args){
        LOG(ERROR,"No worker thread data provided!\n");
        exit(1);
    }

    worker_data_t *data  = (worker_data_t *) args;
    bbuf_t buff = data->bbuf;

    while(1){
        bbuf_remove(buff, &client_fd); // This blocks indefinitely until a request comes
        LOG(INFO,"Processing client request fd %d\n", client_fd);
        
        if(parse_request(client_fd, &request) != 0){
            LOG(ERROR,"Something went wrong parsing HTTP Request\n");
            exit(1); ////////////////////////////////TODO: Instead of killing the entire program immediately, figure out how this thread can signal the others that they should exit once they're done with their request 
        }

        LOG(INFO,"Formulating HTTP response...\n");

        response = get_http_response_from_request(request);

        if (!response){
            LOG(ERROR,"Something went wrong processing HTTP request\n");
            exit(1);
        }

        // Log the response for debugging...
        get_http_response_status(response, status, MAX_RESP_STATUS_LEN);
        get_http_response_headers(response, resp_headers, MAX_RESP_HEADERS_LEN);
        get_http_response_content_size(response, &content_size);
        get_http_response_ressource_fd(response, &ressource_fd);
        
        LOG(DEBUG, "Sending the HTTP response back to the client...\n");
        bytes_written = writen_b(client_fd, status, strlen(status));
        
        if(bytes_written == -1){
            exit(1);
        }

        bytes_written = writen_b(client_fd, resp_headers, strlen(resp_headers));
        
        if(bytes_written == -1){
            exit(1);
        }
        
        if(content_size != 0){
            // Only try to write from reessource fd if there is content to read
            // content size will be 0 in case of errors
            bytes_written = writen(client_fd, ressource_fd, content_size);
            
            if(bytes_written == -1){
                exit(1);
            }
        }

        destroy_http_response(&response);
        destroy_http_request(&request);
        close(client_fd);
    }

    return NULL;
}


static int parse_request(int client_fd, http_req *result){
    http_req tmp_req;
    http_method method;
    char version[MAX_VER_LEN] = {0};
    char uri[MAX_URI_LEN] = {0};

    if(!result){
        LOG(ERROR, "Invalid result reference provided!\n");
        return -1;
    }

    if(client_fd < 0){
        LOG(ERROR, "Bad client file descriptor provided!\n");
        return -1;
    }

    tmp_req = init_http_request(client_fd);
    
    if(!tmp_req){
        LOG(ERROR, "failed to parse request!\n");
        return -1;
    }
    
    get_http_request_method(tmp_req, &method);
    get_http_request_uri(tmp_req, uri);
    get_http_request_version(tmp_req, version);

    LOG(DEBUG, "Got the following request:\n \
    METHOD:   %s\n \
    URI:      %s\n \
    HTTP VER: %s\n", http_method_strings[method], uri, version);

    *result = tmp_req;
    return 0;
}


/**
 * @brief Initialize the worker threads that will handle incoming requests.
 * 
 * @param worker_data reference to data that will get passed to each of the worker threads
 * 
 * @returns true if all worker threads are successfully initialized, otherwise false.
 */
static bool set_up_worker_pool(worker_data_t *worker_data){
    pthread_t tid;
    int rc;

    for(int i=0; i<NUM_WORKER_THREADS; i++){
        if((rc = pthread_create(&tid, NULL, process_incoming_request, (void *)worker_data)) != 0){
            LOG(ERROR, "Something went wrong creating one of the worker threads... got rc %d\n", rc);
            return false;
        }
        LOG(DEBUG, "Created worker thread with thread ID: %lu\n", tid);
    }
    return true; 
}


static void sig_int_handler(int exit_code){
    LOG(INFO, "\nGoodbye...\n");
    exit(EXIT_SUCCESS);
}


static void sig_term_handler(int exit_code){
    LOG(INFO, "\nweb server was terminated!\n");
    exit(EXIT_FAILURE);
}


static void set_up_signal_handlers(){
    signal(SIGTERM, sig_term_handler);
    signal(SIGINT, sig_int_handler);
}


int main(int argc, char *argv[]){
    struct cli *cli_in = calloc(1, sizeof(struct cli));

    set_up_signal_handlers();

    if(!parse_cli(argc, argv, cli_in)){
       return EXIT_FAILURE;
    }

    run_server(cli_in);
    free(cli_in);
    
    return EXIT_SUCCESS;
}