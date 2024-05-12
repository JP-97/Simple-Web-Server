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

struct _http_resp {
    char *status_line;
    char *body;
};

char* html_content = 
    "<!DOCTYPE html>\n"
    "<html lang=\"en\">\n"
    "<head>\n"
    "    <meta charset=\"UTF-8\">\n"
    "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "    <title>Test Page</title>\n"
    "</head>\n"
    "<body>\n"
    "    <h1>Hello, world!</h1>\n"
    "    <p>This is a test page.</p>\n"
    "</body>\n"
    "</html>\n";

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


void destroy_http_request(http_req *request_to_destroy){
    if(!(*request_to_destroy)) return
    
    free(*request_to_destroy);
    *request_to_destroy = NULL;
} 


// RESPONSE //


void init_http_response(http_resp *result){
    
    if(!result)
        return;

    http_resp tmp_response = (http_resp) calloc(1, sizeof(struct _http_resp));
    tmp_response->status_line = (char *) calloc(1, MAX_RESP_STATUS_LEN);
    tmp_response->body = (char *) calloc(1, MAX_RESP_BODY_LEN);

    *result = tmp_response;
}


void destroy_http_response(http_resp *response_to_destroy){
    if(!response_to_destroy)
        return;
    
    free((*response_to_destroy)->status_line);
    free((*response_to_destroy)->body);

    free(*response_to_destroy);
    *response_to_destroy = NULL;
}


int get_http_response_from_request(http_req request_to_process, http_resp response){
    
    if(!request_to_process || !response)
        return -1;

    // TODO Need to add checks here to make sure status and body buffs are big enough!

    strcpy("HTTP/1.0 200 OK\r\n", response->status_line);
    strcpy(html_content, response->body);
    return 0;
}


int get_http_response_status(http_resp response, char *status){
    if(!response || !status)
        return -1;

    status = response->status_line;
    return 0;
}


int get_http_response_body(http_resp response, char *body){
    if(!response || !body)
        return -1;

    body = response->body;
    return 0;
}
