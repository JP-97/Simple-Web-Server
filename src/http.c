#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "http.h"
#include "rio.h"
#define HTTP_REQ_EXPECTED_NUM_ARGUMENTS 3 // Expecting a METHOD, URI and VERSION
#define SERVER_HTTP_VER 1.0

struct _http_req{
    char method[MAX_METHOD_LEN];
    char URI[MAX_URI_LEN];
    char version[MAX_VER_LEN];
};

struct _http_resp {
    char status[MAX_RESP_STATUS_LEN];
    char headers[MAX_RESP_HEADERS_LEN];
    char body[MAX_RESP_BODY_LEN];
    http_response_type response_type;
};

char html_content[] = "<!DOCTYPE html>\r\n"
                      "<html>\r\n"
                      "<head>\r\n"
                      "<title>Hello, World!</title>\r\n"
                      "</head>\r\n"
                      "<body>\r\n"
                        "<h1>Hello, World!</h1>\r\n"
                      "</body>\r\n"
                      "</html>\r\n";

static void http_resp_status_code_to_str(int status_code, char *buff);
static void formulate_full_response(http_req request_to_process, http_resp response);
static void formulate_simple_response(http_req request_to_process, http_resp response);

// REQUEST //

void init_http_request(int client_fd, size_t max_req_len, http_req *result){
    char in_buf[max_req_len];
    int num_vals_scanned = 0;
    http_req tmp_result;
    
    if(!result)
        return;

    *result = NULL;
    tmp_result = (http_req) calloc(1, sizeof(struct _http_req));
    
    if(!tmp_result) 
        return;

    rio_t in_parser = readn_b_init(client_fd);
    memset(in_buf, 0, max_req_len);

    // Read first line from input buffer
    (void) readline_b(in_parser, in_buf, max_req_len-1);
    readn_b_destroy(&in_parser);

    // Parse first line of request
    num_vals_scanned = sscanf(in_buf, "%4s %49s HTTP/%3s", tmp_result->method, tmp_result->URI, tmp_result->version);
    if(num_vals_scanned < HTTP_REQ_EXPECTED_NUM_ARGUMENTS){
        printf("ERROR: Failed to parse HTTP request which should be in format: METHOD URI VERSION\n");
        free(tmp_result);
        return;
    }

    *result = tmp_result;
}


int get_http_request_method(http_req req, char *method){
    if(!req || !method){
        printf("ERROR: invalid argument provided to get_http_request_method...\n");
        return -1;
    }

    strncpy(method, req->method, MAX_METHOD_LEN); //TODO this is pretty dangerous... caller should specify max length (ie size of method buf)
    return 0;
}


int get_http_request_uri(http_req req, char *uri){
    if(!req || !uri){
        printf("ERROR: invalid argument provided to get_http_request_uri...\n");
        return -1;
    }

    strncpy(uri, req->URI, MAX_URI_LEN);
    return 0;
}


int get_http_request_version(http_req req, char *version){
    if(!req || !version){
        printf("ERROR: invalid argument provided to get_http_request_version...\n");
        return -1;
    }
    strncpy(version, req->version, MAX_VER_LEN);
    return 0;
}


void destroy_http_request(http_req *request_to_destroy){
    if(!(*request_to_destroy)) ;

    else {
        free(*request_to_destroy);
        *request_to_destroy = NULL;
    }
    
} 


// RESPONSE //

void init_http_response(http_resp *result){    
    http_resp tmp_response;

    if(!result)
        return;

    *result = NULL;

    tmp_response = (http_resp) calloc(1, sizeof(struct _http_resp));
    
    if(!tmp_response) 
        return;

    *result = tmp_response;
}


void destroy_http_response(http_resp *response_to_destroy){
    if(!response_to_destroy)
        return;

    free(*response_to_destroy);
    *response_to_destroy = NULL;
}


int get_http_response_from_request(http_req request_to_process, http_resp response){

    if(!request_to_process || !response)
    {
        // This needs to provide some internal error response back to client
        formulate_simple_response(request_to_process, response);
    }

    else if(strncmp(request_to_process->version, "0.9", MAX_VER_LEN) == 0)
    {
        formulate_simple_response(request_to_process, response);
    }

    else if ((strncmp(request_to_process->version, "1.0", MAX_VER_LEN) == 0) || 
             (strncmp(request_to_process->version, "1.1", MAX_VER_LEN) == 0))
    {
        formulate_full_response(request_to_process, response);
    }

    else 
    {
        // Error case of unrecognized version
        formulate_full_response(request_to_process, response);
    }

    return 0;
}


int get_http_response_status(http_resp response, char *status, size_t max_status_len){
    if(!response || !status)
        return -1;

    strncpy(status, response->status, min(max_status_len, MAX_RESP_STATUS_LEN));
    return 0;
}


int get_http_response_body(http_resp response, char *body, size_t max_body_len){
    if(!response || !body)
        return -1;

    strncpy(body, response->body, max_body_len);
    return 0;
}


int get_http_response_headers(http_resp response, char *headers, size_t max_header_len){
    if(!response || !headers)
        return -1;

    strncpy(headers, response->headers, max_header_len);
    return 0;
}


int get_http_response_type(http_resp response, int *type){
    if(!response || !type)
        return -1;

    *type = response->response_type;
    return 0;
}

// HELPERS // 

static void http_resp_status_code_to_str(int status_code, char *buff){
    
    if(!buff){
        return;
    }

    switch(status_code){
        case OK:
            strncpy(buff, "OK", 30);
        
        case FILE_NOT_FOUND:   
            strncpy(buff, "Not Found", 30);   
    }
    return;
}


/**
 * @brief Populate http_resp with full response (status and body)
 * 
 * @param request_to_process The client request to process.
 * @param response The response formulated based on the client request.
 */
static void formulate_full_response(http_req request_to_process, http_resp response){
    http_response_type status_code = OK;
    char status_code_str[30];

    printf("DEBUG: Formulating Full HTTP response...\n");

    // Precheck to validate that the request ressource is available
    if(strncmp(request_to_process->URI, "/", MAX_URI_LEN) != 0){
        printf("ERROR: Unrecognized ressource requested -> %s\n", request_to_process->URI);
        status_code = FILE_NOT_FOUND;
    }

    http_resp_status_code_to_str(status_code, status_code_str);

    // Populate the response struct
    snprintf(response->status, MAX_RESP_STATUS_LEN, "HTTP/%.1f %d %s\r\n", SERVER_HTTP_VER, status_code, status_code_str);
    snprintf(response->headers, MAX_RESP_HEADERS_LEN, "Content-type: text/html\r\nContent-length: %ld\r\n\r\n", sizeof(html_content));
    strncpy(response->body, html_content, MAX_RESP_BODY_LEN);

    return;
}


/**
 * @brief Populate http_resp with simple response contents.
 *        Note: For a simple response, this means only sending back the response body.
 * 
 * @param request_to_process The client request to process.
 * @param response The response formulated based on the client request.
 */
static void formulate_simple_response(http_req request_to_process, http_resp response){
    printf("DEBUG: Formulating Simple HTTP response...\n");
    
    strncpy(response->body, html_content, MAX_RESP_BODY_LEN);
    response->response_type = SIMPLE;

    return;
}