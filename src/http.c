#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <regex.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include "http_private.h"
#include "rio.h"

#define MINIMUM_NUM_HTTP_REQ_ARGUMENTS 2 // Expecting at minimum a METHOD and URI (simple request)
#define SERVER_HTTP_VER 1.0

#define RESSOURCE_ROOT "/home/jpoirier/Documents/simple_web_server/ressources" //TODO: This needs to be done via CLI 

#define LEN(arr) sizeof(arr) / sizeof(arr[0])

typedef void (*http_req_validation_func)(http_req, http_resp);

static void http_resp_status_code_to_str(int status_code, char *buff);
static http_method http_method_str_to_enum(const char *method);
static int formulate_full_response(http_req request_to_process, http_resp response);
static int formulate_simple_response(http_req request_to_process, http_resp response);
static void validate_http_method(http_req request_to_process, http_resp response);
static void validate_http_uri(http_req request_to_process, http_resp response);
static void validate_http_version(http_req request_to_process, http_resp response);
static void process_requested_ressource(http_req request_to_process, http_resp response);
static void precheck_request(http_req request_to_process, http_resp response);
static void get_html_content_for_ressource(http_req request, http_resp response);
static int get_ressource_content_type(http_req request, http_resp response);
static void parse_http_method(const char *in_buf, http_req request);
static void parse_http_uri(const char *in_buf, http_req request);
static void parse_http_version(const char *in_buf, size_t in_buf_len, http_req request);
static int get_match_object_len(regmatch_t match);

char *http_response_type_strings[] = {FOREACH_HTTP_METHOD(STRING_GEN)};
char *http_method_strings[] = {FOREACH_HTTP_METHOD(STRING_GEN)};

// REQUEST //

http_req init_http_request(int client_fd, size_t max_req_len){
    char in_buf[max_req_len];
    http_req result;
    
    alloc_http_request(&result);

    if(!result){
        return result;
    }

    // Set request defaults
    // Note: char strings are 0'd already
    // because of calloc, so they're null by default
    result->method = UNKNOWN;

    // Read request from client fd
    rio_t in_parser = readn_b_init(client_fd);
    memset(in_buf, 0, max_req_len);

    (void) readline_b(in_parser, in_buf, max_req_len-1);
    readn_b_destroy(&in_parser);

    parse_http_method(in_buf, result);
    parse_http_uri(in_buf, result);
    parse_http_version(in_buf, max_req_len, result);

    return result;
}


int get_http_request_method(http_req req, http_method *method){
    if(!req || !method){
        printf("ERROR: invalid argument provided to get_http_request_method...\n");
        return -1;
    }

    *method = req->method;

    return 0;
}


int get_http_request_uri(http_req req, char *uri){
    if(!req || !uri){
        printf("ERROR: invalid argument provided to get_http_request_uri...\n");
        return -1;
    }

    strcpy(uri, req->URI);
    return 0;
}


int get_http_request_version(http_req req, char *version){
    if(!req){
        printf("ERROR: invalid argument provided to get_http_request_version...\n");
        return -1;
    }

    else if(!req->version){
        printf("ERROR: Request version on simple request is empty...\n");
        return -1;
    }
    
    strcpy(version, req->version);
    return 0;
}


void destroy_http_request(http_req *request_to_destroy){
    if(!request_to_destroy) return; // Nothing to free...

    free(*request_to_destroy);
    *request_to_destroy = NULL;
    return;
} 


// RESPONSE //

http_resp init_http_response(){    
    http_resp response;

    response = (http_resp) calloc(1, sizeof(struct _http_resp));

    return response;
}


void destroy_http_response(http_resp *response_to_destroy){
    if(!response_to_destroy) return; // Nothing to free...

    free(*response_to_destroy);
    *response_to_destroy = NULL;
    return;
}


int get_http_response_status(http_resp response, char *status, size_t max_status_len){
    if(!response || !status)
        return -1;

    strncpy(status, response->status, max_status_len);
    return 0;
}


int get_http_response_body(http_resp response, char *body, size_t max_body_len){
    if(!response || !body)
        return -1;

    memcpy(body, response->body, max_body_len);
    return 0;
}

int get_http_response_body_size(http_resp response, size_t *body_size){
    if(!response || !body_size) return -1;

    *body_size = (size_t) response->_content_len;
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

/**
 * @brief Get the http response from request object
 * 
 * @note The request will come in one of two formats, FULL or SIMPLE. 
 *       A simple request is in format <"GET" REQUEST_URI>
 *       A full request is in format <METHOD REQUEST_URI HTTP_VERSION>
 *       See RFC1945 for more details. 
 * 
 * @param request_to_process http_req handle for the request that needs processing.
 * @return http_resp handle for populated http response 
 */
http_resp get_http_response_from_request(http_req request_to_process){
    int response_formulated_successfully;
    http_resp response = init_http_response();

    if(!response){
        printf("ERROR: Something went wrong trying to initialize the response!\n");
        return NULL;
    }

    // To start, set return_code to OK
    // This will get modified throughout processing pipeline accordingly
    response->_return_code = OK;

    precheck_request(request_to_process, response);

    if (response->_return_code != OK){
        goto generate_response;
    }

    // Process the ressource
    get_html_content_for_ressource(request_to_process, response);

    // Set status last after we've had a chance to process ressource

    generate_response:
        switch(response->response_type){
            case FULL:
                response_formulated_successfully = formulate_full_response(request_to_process, response);
                break;

            case SIMPLE:
                response_formulated_successfully = formulate_simple_response(request_to_process, response);
                break;
        
            default:
                printf("ERROR: Unrecognized response type... Internal error\n");
                return NULL;
        }

    return (response_formulated_successfully == 0) ? response : NULL;
}


// HELPERS //


void alloc_http_request(http_req *result){
    if(!result){
        printf("ERROR: Bad http_req reference provided\n");
        return;
    }

    http_req tmp = (http_req) calloc(1, sizeof(struct _http_req));
    
    *result = tmp ? tmp : NULL;
    
    return;
}

/**
 * @brief Convert the http request method str to its enum representation.
 * 
 * @param method int containing the status code to convert (eg: 404).
 */
static http_method http_method_str_to_enum(const char *method){
    http_method m = 0;
    for(; m < HTTP_METHOD_MAX; m++){
        if(strncmp(method, http_method_strings[m], MAX_METHOD_LEN) == 0){
            break;
        }
    }

    return m;
}


/**
 * @brief Convert the int HTTP status code to its string representation.
 * 
 * @param status_code int containing the status code to convert (eg: 404).
 * @param buff reference containing location where result should be stored.
 */
static void http_resp_status_code_to_str(int status_code, char *buff){
    
    if(!buff){
        return;
    }

    switch(status_code){
        case OK:
            strncpy(buff, "OK", 30);
            break;
        
        case BAD_REQUEST:
            strncpy(buff, "Bad Request", 30);
            break;

        case UNAUTHORIZED:
            strncpy(buff, "Unauthorized", 30);
            break;

        case FILE_NOT_FOUND:   
            strncpy(buff, "Not Found", 30);
            break;

        case NOT_IMPLEMENTED:
            strncpy(buff, "Not Imeplemented", 30);  
            break;
        
        case INTERNAL_ERROR:
            strncpy(buff, "Internal server error", 30);
            break;
    
        case UNSUPPORTED_VER:
            strncpy(buff, "Unsupported request version", 30);
            break;

    }
    return;
}


/**
 * @brief Populate http_resp with full response (status and body)
 * 
 * @param request_to_process The client request to process.
 * @param response The response formulated based on the client request.
 *
 * @return 0 on success, otherwise -1.
 */
static int formulate_full_response(http_req request_to_process, http_resp response){
    char status_code_str[30];
    char content_type[MAX_RES_TYPE_LEN];

    if(!response){
        printf("ERROR: Null response provided... returning without populating response\n");
        return -1;
    }

    if(!request_to_process){
        response->_return_code = BAD_REQUEST;
    }
    
    printf("DEBUG: Formulating Full HTTP response...\n");

    if(strlen(response->body) == 0 && response->_return_code == OK){
        // Empty body means something went wrong getting ressource...
        response->_return_code = INTERNAL_ERROR;
    }

    http_resp_status_code_to_str(response->_return_code, status_code_str);
   
    // Populate the HTTP response status line
    // HTTP-Version Status-Code Reason-Phrase
    snprintf(response->status, MAX_RESP_STATUS_LEN, "HTTP/%.1f %d %s\r\n", SERVER_HTTP_VER, response->_return_code, status_code_str);

    // Populate headers
    // TODO Need to clean this up - maybe a macro? 
    
    if(response->_return_code != OK || strlen(response->body) == 0){
        // For a bad request where body is empty, we don't need headers
        return 0;
    }

    if(get_ressource_content_type(request_to_process, response) != 0){
        response->_return_code = INTERNAL_ERROR;
        return 0;
    }

    snprintf(response->headers, MAX_RESP_HEADERS_LEN, "Content-length: %ld\r\nContent-type: %s\r\n\r\n", response->_content_len, response->_content_type);
    
    // snprintf(response->headers, MAX_RESP_HEADERS_LEN, "Content-type: %s\r\n\r\n", content_type);

    return 0;
}


/**
 * @brief Populate http_resp with simple response contents.
 *        Note: For a simple response, this means only sending back the response body.
 *              A simple response doesn't have any headers.
 * 
 * @param request_to_process The client request to process.
 * @param response The response formulated based on the client request.
 */
static int formulate_simple_response(http_req request_to_process, http_resp response){
    printf("DEBUG: Formulating Simple HTTP response...\n");
    return 0;
}


/**
 * @brief Perform some basic left-to-right prechecks against the provided request.
 * 
 * @param request_to_process request to validate.
 * @param response response to update based on precheck results.
 */
static void precheck_request(http_req request_to_process, http_resp response){
    if(!response){
        printf("ERROR: Null response object provided!\n");
        return;
    }

    else if(!request_to_process){
        printf("ERROR: Internal error\n");
        response->_return_code = INTERNAL_ERROR;
        return;
    }

    response->response_type = (strlen(request_to_process->version) == 0) ? SIMPLE : FULL;

    http_req_validation_func validation_functions[] = {validate_http_method, 
                                                       validate_http_uri, 
                                                       validate_http_version};


    for(int i = 0; i < LEN(validation_functions); i++){
        validation_functions[i](request_to_process, response);
    }
}


static void validate_http_method(http_req request_to_process, http_resp response)
{
    if(request_to_process->method == UNKNOWN){
        printf("ERROR: Empty request method... invalid request!\n");
        response->_return_code = BAD_REQUEST;
        return;
    }

    else if(response->response_type == SIMPLE && request_to_process->method != GET){
        printf("ERROR: method not supported for simple request\n");
        response->_return_code = BAD_REQUEST;
        return;
    }
}


static void validate_http_uri(http_req request_to_process, http_resp response)
{
    snprintf(request_to_process->_ressource_abs_path,
             MAX_SERVER_ROOT_LEN + MAX_URI_LEN,
             "%s%s%s",
             server_root_location,
             request_to_process->_ressource_location,
             strcmp(request_to_process->_ressource_name, "/") == 0 ? "/index.html" : request_to_process->_ressource_name);
    
    printf("Ressource full path: %s\n", request_to_process->_ressource_abs_path);

    if(access(request_to_process->_ressource_abs_path, F_OK) != 0){
        printf("ERROR: Could not access %s\n", request_to_process->_ressource_abs_path);
        response->_return_code = FILE_NOT_FOUND;
        return;
    }

    else if(access(request_to_process->_ressource_abs_path, R_OK) != 0){ // TODO: For now, only checking read permissions, but this will change when we add executables
        printf("ERROR: Bad permissions for requested ressource %s\n", request_to_process->_ressource_abs_path);
        response->_return_code = UNAUTHORIZED;
        return;
    }
}


/**
 * @brief Validate that the request_to_process has a valid http version
 * 
 * @note In the case of a simple request, return - since there's no 
 *       expected version corresponding to this request.
 * 
 * @param request_to_process request to validate.
 * @param response response to update based on validation.
 */
static void validate_http_version(http_req request_to_process, http_resp response)
{
    regex_t re_valid_request;
    regex_t re_full;
    char *valid_request_pat = "[1-9]\\.[0-9]";
    char *full_response_pat = "1\\.[0-1]";
    regmatch_t re_valid_req_result[1];
    regmatch_t re_full_result[1];

    if(response->response_type == SIMPLE){
        // No validation to do against simple request
        return;
    }
    
    int status_valid_req = regcomp(&re_valid_request, valid_request_pat, 0);
    int status_full = regcomp(&re_full, full_response_pat, 0);

    if(status_valid_req != 0 || status_full != 0){
        response->_return_code = INTERNAL_ERROR;
        goto clean_up;
    }

    else if(regexec(&re_valid_request, request_to_process->version, 1, re_valid_req_result, 0) == REG_NOMATCH){
        printf("ERROR: malformed request... invalid http version\n");
        response->_return_code = BAD_REQUEST;
        goto clean_up;
    }

    else if(regexec(&re_full, request_to_process->version, 1, re_full_result, 0) == REG_NOMATCH){
        printf("ERROR: Unsupported request version\n");
        // Currently only supporting Full requests for version 1.0 and 1.1
        response->_return_code = UNSUPPORTED_VER;
    }

    clean_up:
        regfree(&re_valid_request);
        regfree(&re_full);
}


/**
 * @brief Read the contents of the requested ressource into the response's body buffer.
 * 
 * @param request Request to process.
 * @param response Response being updated.
 */
static void get_html_content_for_ressource(http_req request, http_resp response)
{
    if(!request || !response){
        printf("ERROR: Invalid buffer reference provided for fetching html content.\n");
        return;
    }

    rio_t ressource_handle = NULL;

    printf("Fetching contents for %s\n", request->_ressource_abs_path);
    int ressource_fd = open(request->_ressource_abs_path, O_RDONLY);

    if(!ressource_fd){
        printf("ERROR: Failed to open file descriptor for requested ressource %s\n", request->_ressource_abs_path);
        goto clean_up;
    }

    ressource_handle = readn_b_init(ressource_fd);
    ssize_t num_bytes_read = readn_b(ressource_handle, response->body, MAX_RESP_BODY_LEN);
    
    if(num_bytes_read == -1){
        printf("ERROR: Failed to fetch html contents for %s\n", request->_ressource_abs_path);
    }

    response->_content_len = num_bytes_read;

    clean_up:
        if(ressource_handle) readn_b_destroy(&ressource_handle);
        close(ressource_fd);
    
    return;
}


/**
 * @brief Provide the content type needed in the response based on the provided request.
 * 
 * @param request request which contains ressource absolute path.
 * @param ressource_type_result buffer to store the ressource type.
 */
static int get_ressource_content_type(http_req request, http_resp response)
{
    ext_map recognized_ext_mappings[NUM_RECOGNIZED_EXT_MAPPINGS] = {{.extension = ".html",
                                                                     .content_type = "text/html"},
                                                                     {.extension = ".jpeg",
                                                                      .content_type = "image/jpeg"}};

    const char *default_content_type = "text/plain";


    if(!request || !response){
        printf("ERROR: Invalid input for getting ressource type!\n");
        return -1;
    }

    char *requested_ressource_extension_start = strstr(request->_ressource_abs_path, ".");

    for(int i = 0; i < NUM_RECOGNIZED_EXT_MAPPINGS; i++){

        if(strncmp(requested_ressource_extension_start, recognized_ext_mappings[i].extension, MAX_RES_EXT_LEN) !=0){
            continue;    
        }

        strncpy(response->_content_type, recognized_ext_mappings[i].content_type, MAX_RES_TYPE_LEN-1);
        printf("%s is of Content-Type: %s\n", request->_ressource_abs_path, recognized_ext_mappings[i].content_type);
        return 0;
    }

    printf("WARNING: Could not determine content type for requested ressource: %s... using default\n", request->_ressource_abs_path);
    strncpy(response->_content_type, default_content_type, MAX_RES_TYPE_LEN -1);
    return 0;
}



static void parse_http_method(const char *in_buf, http_req request)
{
    char *result;
    
    if(!in_buf){
        printf("ERROR: Invalid request input\n");
        return;
    }

    // Iterate through supported methods
    for(int method = GET; method < HTTP_METHOD_MAX; method++){
        result = strstr(in_buf, http_method_strings[method]);
        if(!result){
            // No match found...
            continue;
        }

        request->method = method;
        return;
    }

    request->method = UNKNOWN;
}


static void parse_http_uri(const char *in_buf, http_req request){
    regex_t re_valid_request_uri;
    regmatch_t re_request_uri_result[3];
    char request_uri_pat[MAX_URI_LEN] = {0};
    int ressource_location_match_group = 1;
    int ressource_name_match_group = 2;

    char *request_scheme = "https?";
    char *domain_or_ip_pat = "[^/]+";
    char *ressource_pat = "/.*";
    
    // Note: Request could be absolute or relative
    snprintf(request_uri_pat, MAX_URI_LEN, "(%s://%s)?(%s) ", request_scheme, domain_or_ip_pat, ressource_pat);

    if(!in_buf){
        printf("ERROR: Invalid request input\n");
        return;
    }

    else if(regcomp(&re_valid_request_uri, request_uri_pat, REG_EXTENDED) != 0){
        printf("ERROR: Failed to compile request uri pattern\n");
        goto clean_up;
    }
    
    else if(regexec(&re_valid_request_uri, in_buf, 3, re_request_uri_result, 0) != 0){
        printf("ERROR: Invalid URI in request: %s\n", in_buf);
        goto clean_up;
    }

    int uri_len = get_match_object_len(re_request_uri_result[0]);
    uri_len = uri_len > MAX_URL_LEN ? MAX_URL_LEN : uri_len;

    int ressource_loc_len = get_match_object_len(re_request_uri_result[ressource_location_match_group]);
    ressource_loc_len = ressource_loc_len > MAX_RESSOURCE_LEN ? MAX_RESSOURCE_LEN : ressource_loc_len;
    
    int ressource_name_len = get_match_object_len(re_request_uri_result[ressource_name_match_group]);
    ressource_name_len = ressource_name_len > MAX_RESSOURCE_LEN ? MAX_RESSOURCE_LEN : ressource_name_len;

    strncpy(request->URI, in_buf + re_request_uri_result[0].rm_so, uri_len);

    //TODO: For now this assumes that the incoming request will always be relative
    // Eventually we need to fix this (ressource location is just the protocol and the location...)

    strncpy(request->_ressource_location, in_buf + re_request_uri_result[ressource_location_match_group].rm_so, ressource_loc_len);
    strncpy(request->_ressource_name, in_buf + re_request_uri_result[ressource_name_match_group].rm_so, ressource_name_len);

    clean_up:
        regfree(&re_valid_request_uri);
}


static void parse_http_version(const char *in_buf, size_t in_buf_len, http_req request)
{
    regex_t re_valid_version;
    regmatch_t re_request_version_result[2];
    char in_buf_cpy[in_buf_len];
    char *version_pat = "([0-9]\\.[0-9])\r\n$";

    if(!in_buf || in_buf_len == 0){
        printf("ERROR: Invalid request input\n");
        return;
    }

    strncpy(in_buf_cpy, in_buf, in_buf_len-1);

    if(regcomp(&re_valid_version, version_pat, REG_EXTENDED) != 0){
        printf("ERROR: Failed to compile request method pattern\n");
        return;
    }

    else if(regexec(&re_valid_version, in_buf_cpy, 2, re_request_version_result, 0) != 0){
        printf("ERROR: Failed to find a method in provided request\n");
        regfree(&re_valid_version);
        return;
    }

    int version_match_len = get_match_object_len(re_request_version_result[1]);
    printf("%d\n", version_match_len);
    version_match_len = version_match_len > MAX_VER_LEN ? MAX_VER_LEN : version_match_len;

    strncpy(request->version, in_buf + re_request_version_result[1].rm_so, version_match_len);
    regfree(&re_valid_version);
}


static void process_requested_ressource(http_req request_to_process, http_resp response);


static int get_match_object_len(regmatch_t match){
    int last_char_offset = (int) match.rm_eo; // rm_eo is the index of the first char AFTER the match is done
    return last_char_offset - (int) match.rm_so;
}
