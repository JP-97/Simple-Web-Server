#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "command_line.h"
#include "rio.h"
#include "http.h"
#include "bbuf.h"
#include "log.h"

#define BUFF_SIZE 100 // TODO need to optimize this
#define MAX_BBUFF_LEN 25
#define MAX_SERVER_HOSTNAME_LEN 25
#define NUM_WORKER_THREADS 5
#define NUM_RECOGNIZED_SIGS 2
#define NANOSEC_IN_SEC 1000000000⁠
#define MAX_SERVER_SHUTDOWN_TIME 10

sig_atomic_t g_server_running = 0; // Used to coordinate server event loop shutdown


typedef struct _server_context_t {
    bbuf_t bbuf;
    int server_fd;
    pthread_t worker_tids[NUM_WORKER_THREADS];
    sem_t shutdown_complete; // Synchronize main and monit thread during controlled shutdown
    bool shutdown_was_clean;
    pthread_t monit_thread_tid;
} server_context_t;



static void run_server(struct cli *cli_in);
static bool set_up_monit_thread(server_context_t *worker_data);
static bool set_up_worker_pool(server_context_t *worker_data);
static int bind_server_port(int server_port, int *svr_fd, char *server_ip);
static int parse_request(int client_fd, http_req *result);
static void *process_incoming_request(void *args);
static void *handle_controlled_shutdown_req(void *args);
static bool populate_sigset(sigset_t *set_to_populate);
static bool prevent_controlled_shutdown();


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

    server_context_t worker_data;
    memset(&worker_data, 0, sizeof(server_context_t));

    if(!cli_in){
        LOG(ERROR,"Failed in initializing internal data structures - Aborting launch!");
        goto exit_on_failure;
    }

    LOG(INFO, "Initializing server with root directory: %s\n", cli_in->server_root);

    // Set up bounded buffer and worker pool
    bbuf_t bbuf = bbuf_init();

    if(!bbuf){
        goto exit_on_failure;
    }

    // Note: All worker threads are initialized with same
    // bbuf since the buffer is specifically built to operate with
    // multiple users and synchronizes the access internally.
    // worker_data can be stored on the stack because this function
    // survives for the lifeftime of the program.
    worker_data.bbuf = bbuf;

    if(!set_up_worker_pool(&worker_data)){
        goto exit_on_failure;
    }

    else if(bind_server_port(cli_in->port, &server_fd, host_name) != 0){
        LOG(ERROR,"Failed to bind server to port %d\n", cli_in->port);
        goto exit_on_failure;
    }

    worker_data.server_fd = server_fd;

    if(!set_up_monit_thread(&worker_data)){
        goto exit_on_failure;
    }

    LOG(INFO,"Successfully initialized server at %s:%d\n" \
             "Enter CTRL+C to kill the server...\n", host_name, cli_in->port);
    
    free(cli_in);
    g_server_running = 1;

    // Enter main loop and listen for incoming connections
    while(g_server_running){
        client_con_size = sizeof(client_con);

        client_fd = accept(server_fd, (struct sockaddr*) &client_con, &client_con_size);
        
        if(errno == EBADF || errno == EINVAL){
            // Handle the case where monit thread closes server_fd but
            // main thread is "inside" the accept call 
            LOG(DEBUG, "Exiting from server event loop...\n");
            break;
        }

        else if (client_fd == -1){
            LOG(ERROR,"Could not accept client connection due to: %s\n", strerror(errno));
            goto exit_on_failure;
        }

        // Get human readable conversion of client connection info
        got_info = getnameinfo((struct sockaddr*)&client_con, client_con_size, client_addr, BUFF_SIZE, NULL, 0, NI_NUMERICHOST);
        if (got_info != 0){
            LOG(ERROR,"Could not determine human readable conversion of client info due to: %s\n", strerror(errno));
            goto exit_on_failure;
        }

        LOG(INFO,"Successfully established connection with %s!\n", client_addr);

        // Add the client FD to bounded buffer so it can be handled by one of workers
        if (bbuf_insert(bbuf, client_fd) != 0){
            LOG(ERROR,"WARNING: Failed to insert client connection into buffer...\n");
            continue;
        };
    }

    // If we made it to here, the monit thread is handling
    // graceful shutdown request and killed the server event loop.
    // The reason for handling this inside the run_server function is
    // because the bounded buffer and other datastructures are
    // local to this function
    struct timespec server_shutdown_timeout;

    if (clock_gettime(CLOCK_REALTIME, &server_shutdown_timeout) == -1) {
        LOG(ERROR, "Could not get current time - aborting controlled shutdown");
        goto exit_on_failure;
    }

    server_shutdown_timeout.tv_sec += MAX_SERVER_SHUTDOWN_TIME;
    
    if(sem_timedwait(&(worker_data.shutdown_complete), &server_shutdown_timeout) != 0){
        LOG(ERROR, "Failed waiting for server to shutdown... potentially ungraceful exit\n");
        goto exit_on_failure;
    }
    
    worker_data.shutdown_was_clean ? exit(EXIT_SUCCESS) : exit(EXIT_FAILURE);

    exit_on_failure:
        exit(EXIT_FAILURE);
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
 *       exit until the server as a whole is shut down. Once a request
 *       is processed, the worker thread re-enters its loop where it monitors
 *       for new client FDs added to the bounded buffer.
 * 
 * @note If an error is encountered while handling the request, the server will
 *       attempt to respond with the appropriate HTTP status code. If this is not possible,
 *       the server will close the client fd such, completing the request. 
 * 
 * @param args input data for the callback
 * @return void* NULL or exits.
 */
static void *process_incoming_request(void *args){
    int client_fd;
    int ressource_fd;
    char status[MAX_RESP_STATUS_LEN];
    char resp_headers[MAX_RESP_HEADERS_LEN];
    off_t content_size;
    http_req request = NULL;
    http_resp response = NULL;
    ssize_t bytes_written;

    if(!args){
        LOG(ERROR,"No worker thread data provided!\n");
        exit(EXIT_FAILURE); // Extreme case - can't signal monit thread so exit directly
    }

    server_context_t *data  = (server_context_t *) args;
    bbuf_t buff = data->bbuf;

    while(1){

        // Block indefinitely until a client_fd is added to the buffer
        // or the server is shutdown
        bbuf_remove(buff, &client_fd); 

        LOG(INFO,"Processing client request fd %d\n", client_fd);

        // Prevent cancellation while handling request
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

        if(parse_request(client_fd, &request) != 0){
            LOG(ERROR,"Something went wrong parsing HTTP Request\n");
            goto clean_up;
        }

        LOG(INFO,"Formulating HTTP response...\n");

        response = get_http_response_from_request(request);

        if (!response){
            LOG(ERROR,"Something went wrong processing HTTP request\n");
            goto clean_up;
        }

        // Log the response for debugging...
        get_http_response_status(response, status, MAX_RESP_STATUS_LEN);
        get_http_response_headers(response, resp_headers, MAX_RESP_HEADERS_LEN);
        get_http_response_content_size(response, &content_size);
        get_http_response_ressource_fd(response, &ressource_fd);
        
        LOG(DEBUG, "Sending the HTTP response back to the client...\n");
        shutdown(client_fd, SHUT_RD);
        bytes_written = writen_b(client_fd, status, strlen(status));
        
        if(bytes_written == -1){
            goto clean_up;
        }

        bytes_written = writen_b(client_fd, resp_headers, strlen(resp_headers));
        
        if(bytes_written == -1){
            goto clean_up;
        }
        
        if(content_size != 0){
            // Only try to write from reessource fd if there is content to read
            // content size will be 0 in case of errors
            bytes_written = writen(client_fd, ressource_fd, content_size);
            
            if(bytes_written == -1){
                goto clean_up;
            }
        }

        clean_up:
            shutdown(client_fd, SHUT_WR);
            close(client_fd);
            destroy_http_request(&request);
            destroy_http_response(&response);

        // Re-enable cancellation now that request handling is done.
        // This will trigger thread shutdown if any signals were queued
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
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
static bool set_up_worker_pool(server_context_t *worker_data){
    pthread_t tid;
    int rc;

    for(int i=0; i<NUM_WORKER_THREADS; i++){
        if((rc = pthread_create(&tid, NULL, process_incoming_request, (void *)worker_data)) != 0){
            LOG(ERROR, "Something went wrong creating one of the worker threads... got rc %d\n", rc);
            return false;
        }

        worker_data->worker_tids[i] = tid;
        LOG(DEBUG, "Created worker thread with thread ID: %lu\n", tid);
    }

    return true; 
}


/**
 * @brief Set the up monitor thread. This thread is responsible
 *        for ensuring gracecful shutdown in event of SIGINT or SIGTERM
 * 
 * @param worker_data contains a reference to server_context_t struct which 
 *                    holds worker thread IDs.
 * 
 * @returns true on successfully creating monit thread, otherwise false.
 */
static bool set_up_monit_thread(server_context_t *worker_data){
    pthread_t monit_tid;
    pthread_attr_t monit_attrs;
    int rc;

    if(pthread_attr_setdetachstate(&monit_attrs, PTHREAD_CREATE_DETACHED) != 0){
        LOG(ERROR, "Failed in setting monit thread attributes!\n");
        return false;
    }
    else if((rc = pthread_create(&monit_tid, NULL, handle_controlled_shutdown_req, worker_data)) != 0){
        LOG(ERROR, "Failed in creating monit thread!\n - error: %d", rc);
        return false;
    }
    
    LOG(DEBUG, "Created monit thread with thread ID: %lu\n", monit_tid);
    worker_data->monit_thread_tid = monit_tid;
    return true;   
}


/**
 * @brief Wait for SIGINT/SIGTERM and cancel/join all worker threads
 * 
 * @param args contains struct with worker thread IDs
 */
static void *handle_controlled_shutdown_req(void *args){
    sigset_t set;
    int received_sig;
    int rc;
    int client_fd;
    void *thread_result;
    struct timespec thread_shutdown_timeout;
    ssize_t bytes_written;
    char status[MAX_RESP_STATUS_LEN];
    char resp_headers[MAX_RESP_HEADERS_LEN];
    http_resp response = NULL;
    
    server_context_t *server_data = (server_context_t *) args;
    server_data->shutdown_was_clean = true;
    
    sem_init(&(server_data->shutdown_complete), 0, 0);

    populate_sigset(&set);

    // Block on "handled" signals defined in 'set'
    sigwait(&set, &received_sig);

    LOG(DEBUG, "\n\nHandling signal: %s\n", received_sig == SIGINT ? "SIGINT" : "SIGTERM");

    // Stop accepting new requests on server fd
    shutdown(server_data->server_fd, SHUT_RD);
    g_server_running = 0;

    // Shut down all worker threads
    for(int i=0; i<NUM_WORKER_THREADS; i++){
        LOG(DEBUG, "Shutting down worker thread %lu\n", server_data->worker_tids[i]);
        rc = pthread_cancel(server_data->worker_tids[i]);

        if(rc == ESRCH){
            LOG(WARNING, "OS Could not find thread with ID: %lu", server_data->worker_tids[i]);
        }
    }

    LOG(DEBUG, "Sent cancel signal to all worker threads - beginning thread join...\n");
    
    if (clock_gettime(CLOCK_REALTIME, &thread_shutdown_timeout) == -1) {
        LOG(ERROR, "Failed to get realtime clock value...");
        goto return_control_to_main_thread; 
    }

    // Allocate a 10% buffer for overall server to shut down 
    // on top of what's handled within monit thread
    int thread_join_timeout = MAX_SERVER_SHUTDOWN_TIME-(MAX_SERVER_SHUTDOWN_TIME/10);
    thread_shutdown_timeout.tv_sec += thread_join_timeout; 

    for(int i=0; i<NUM_WORKER_THREADS; i++){
        rc = pthread_timedjoin_np(server_data->worker_tids[i], 
                                  &thread_result, 
                                  &thread_shutdown_timeout);

        if(rc == 0 && thread_result == PTHREAD_CANCELED){
            LOG(DEBUG, "Successfully joined worker thread %lu\n", server_data->worker_tids[i]);
        }

        else if(rc == EBUSY || rc == ETIMEDOUT){
            LOG(ERROR, "Thread %lu still not joined after %ds - proceeding with cleanup...!\n", 
                                                            server_data->worker_tids[i],
                                                            thread_join_timeout);
            server_data->shutdown_was_clean = false;
            
        }

        else {
            LOG(ERROR, "Unexpected error encountered trying to join worker thread %lu - got rc: %d\n", 
                                                                        server_data->worker_tids[i],
                                                                        rc);
            server_data->shutdown_was_clean = false;
        }
    }
   
    LOG(DEBUG, "Replying to pending client fds that server is shutting down...\n");
    while(get_bbuf_items(server_data->bbuf) > 0){

        if(bbuf_remove(server_data->bbuf, &client_fd) != 0){
            LOG(ERROR, "Something went wrong replying to pending client requests...");
            continue;
        }

        if(!response){
            response = get_server_shutting_down_response();
        }

        // TODO: Move all of this logic to a helper since it's duplicated again in normal path
        LOG(DEBUG, "Sending shutting down response back to the client fd %d\n", client_fd);
        shutdown(client_fd, SHUT_RD);
        get_http_response_status(response, status, MAX_RESP_STATUS_LEN);
        get_http_response_headers(response, resp_headers, MAX_RESP_HEADERS_LEN);
        
        bytes_written = writen_b(client_fd, status, strlen(status));
        
        if(bytes_written == -1){
            LOG(ERROR, "Failed to write status line back to client on fd %d\n", client_fd);
            server_data->shutdown_was_clean = false;
            goto cleanup_response;
        }

        bytes_written = writen_b(client_fd, resp_headers, strlen(resp_headers));
        
        if(bytes_written == -1){
            LOG(ERROR, "Failed to write headers back to client on fd %d\n", client_fd);
            server_data->shutdown_was_clean = false;
            goto cleanup_response;
        }

        LOG(DEBUG, "Successfully sent shutting down notification to client on fd %d\n", client_fd);
        shutdown(client_fd, SHUT_WR);

        cleanup_response:
            close(client_fd);
            destroy_http_response(&response);
    }

    shutdown(server_data->server_fd, SHUT_WR);
    close(server_data->server_fd);

    return_control_to_main_thread:
        LOG(DEBUG, "Monit thread finished shutting down %s - returning control back to main thread to complete shutdown!\n", 
                    server_data->shutdown_was_clean ? "successfully" : "but encountered some issues");
        sem_post(&(server_data->shutdown_complete));
}


/**
 * @brief Set up calling thread to block SIGINT and SIGTERM
*/
static bool prevent_controlled_shutdown(){
    sigset_t set;

    if(!populate_sigset(&set)){
        return false;
    }

    else if(pthread_sigmask(SIG_BLOCK, &set, NULL) != 0){
        LOG(ERROR, "Failed in initializing internal datastructures - aborting launch...\n");
        return false; 
    }

    return true;
}


/**
 * @brief Populate provided sigset_t reference with the program's recognized signals.
 * 
 * @param set_to_populate reference to sigset_t to populate
 * @return true if signal set initialized successfully, else false.
 */
static bool populate_sigset(sigset_t *set_to_populate){
    int signals_to_mask[NUM_RECOGNIZED_SIGS] = {SIGINT, SIGTERM};

    if(sigemptyset(set_to_populate) != 0){
        LOG(ERROR, "Failed in initializing server datastructures - aborting launch...");
        return false;
    }

    for(int i = 0; i < NUM_RECOGNIZED_SIGS; i++){
        if(sigaddset(set_to_populate, signals_to_mask[i]) != 0){
            LOG(ERROR, "Failed in initializing internal datastructures - aborting launch...");
            return false;
        }
    }

    return true;
}


int main(int argc, char *argv[]){
    struct cli *cli_in = calloc(1, sizeof(struct cli));
    
    // Temporarily queue "controlled shutdown" signals
    // until we've finished initializing
    if(!prevent_controlled_shutdown()){
        return EXIT_FAILURE;
    }
    else if(!parse_cli(argc, argv, cli_in)){
       return EXIT_FAILURE;
    }

    run_server(cli_in);
}