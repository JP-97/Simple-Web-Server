#include <string.h>
#include <stdio.h>
#include "http.h"
#include "rio.h"
#define HTTP_REQ_EXPECTED_NUM_ARGUMENTS 3 // Expecting a METHOD, URI and VERSION

struct _http_req{
    char method[MAX_METHOD_LEN];
    char URI[MAX_URI_LEN];
    char version[MAX_VER_LEN];
};


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
    memset(in_buf, 0, sizeof(in_buf));

    // Read first line from input buffer
    int bytes_read = readline_b(in_parser, in_buf, max_req_len-1);
    in_buf[bytes_read+1] = '\0'; // Null terminate so we can treat as a string
    readn_b_destroy(&in_parser);

    // Parse first line
    num_vals_scanned = sscanf(in_buf, "%4s %49s HTTP/%3s", tmp_result->method, tmp_result->URI, tmp_result->version);
    if(num_vals_scanned < HTTP_REQ_EXPECTED_NUM_ARGUMENTS){
        printf("ERROR: Failed to parse HTTP request which should be in format: METHOD URI VERSION\n");
        free(tmp_result);
        return;
    }

    tmp_result->method[MAX_METHOD_LEN-1] = '\0';
    tmp_result->URI[MAX_URI_LEN-1] = '\0';
    tmp_result->version[MAX_VER_LEN-1] = '\0';

    *result = tmp_result;
}


int get_http_request_method(http_req req, char *method){
    if(!req || !method){
        printf("ERROR: invalid argument provided to get_http_request_method...\n");
        return -1;
    }

    strncpy(method, req->method, MAX_METHOD_LEN);
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


int get_http_response_from_request(http_req request_to_process, http_resp **http_response){
    
    if(!request_to_process || !(*http_response))
        return -1;

    (*http_response)->status_line = "HTTP/1.0 404 ";
    return 0;
}



void destroy_http_request(http_req *request_to_destroy){
    if(!(*request_to_destroy)) return
    
    free(*request_to_destroy);
    *request_to_destroy = NULL;
} 