/**
 * @file http.h
 * @author Josh Poirier
 * @date 4 Apr 2024
 * @brief File containing some helper APIs for handling HTTP requests.
 *
 */

#ifndef _HTTP
#define _HTTP

#include <stdlib.h>
#include "command_line.h"

#define MAX_METHOD_LEN        5
#define MAX_URL_LEN           50
#define MAX_RESSOURCE_LEN     50
#define MAX_URI_LEN           MAX_URL_LEN + MAX_RESSOURCE_LEN
#define MAX_VER_LEN           4  // 1.0, 1.1, etc. 
#define MAX_RES_EXT_LEN       10
#define MAX_RES_TYPE_LEN      30

#define MAX_RESP_STATUS_LEN  60
#define MAX_RESP_HEADERS_LEN 500

#define ENUM_GEN(ENUM) ENUM,
#define STRING_GEN(STRING) #STRING,

// Note: If you need to add new elems to FOREACH macros, only append, don't insert

#define FOREACH_HTTP_RESPONSE_TYPE(RESPONSE_TYPE)           \
                            RESPONSE_TYPE(SIMPLE)           \
                            RESPONSE_TYPE(FULL)             \
                            RESPONSE_TYPE(RESPONSE_TYPE_MAX) 

#define FOREACH_HTTP_METHOD(HTTP_METHOD)                \
                    HTTP_METHOD(GET)                    \
                    HTTP_METHOD(HEAD)                   \
                    HTTP_METHOD(POST)                   \
                    HTTP_METHOD(UNKNOWN)                \
                    HTTP_METHOD(HTTP_METHOD_MAX)     
        

typedef struct _http_req *http_req;
typedef struct _http_resp *http_resp;


typedef enum _http_return_code {
    OK              = 200,
    BAD_REQUEST     = 400,
    UNAUTHORIZED    = 401, 
    FILE_NOT_FOUND  = 404,
    INTERNAL_ERROR  = 500,
    NOT_IMPLEMENTED = 501,
    UNSUPPORTED_VER = 505

} http_return_code;

typedef enum _http_response_type  {
    FOREACH_HTTP_RESPONSE_TYPE(ENUM_GEN)
} http_response_type;

extern char *http_response_type_strings[];

typedef enum _http_method {
    FOREACH_HTTP_METHOD(ENUM_GEN)
} http_method;

extern char *http_method_strings[];

/**
 * @brief Parse http request on client_fd.
 * 
 * @return http_req handler for request object.
*/
http_req init_http_request(int client_fd);


/**
 * @brief Initializer for HTTP response
 * 
 * @return http_resp handler for response object.
 */
http_resp init_http_response();


/**
 * @brief Free all memory allocated to provided http_req object.
*/
void destroy_http_request(http_req *request_to_destroy);


/**
 * @brief Free all memory allocated to provided http_req object.
*/
void destroy_http_response(http_resp *response_to_destroy);


/**
 * @brief return the http method specified in the incoming request.
 * 
 * @param req Pointer to initialized http request
 * @param method pointer to location where to store result.
 */
int get_http_request_method(http_req req, http_method *method);


/**
 * @brief return the URI specified in the incoming request.
 * 
 * @param req Pointer to initialized http request
 * @param uri Pointer to location where to store result.
 */
int get_http_request_uri(http_req req, char *uri);


/**
 * @brief return the http version specified in the incoming request.
 * 
 * @param req Pointer to initialized http request
 * @param version pointer to location where to store result.
 */
int get_http_request_version(http_req req, char *version);


/**
 * @brief Get an http response based on an http_request
 * 
 * @param request_to_process Pointer to http_request to process 
 * 
 * @return http_resp the initialized http response handler.
 */
http_resp get_http_response_from_request(http_req request_to_process);


/**
 * @brief Get the http response status
 * 
 * @param response the response from which to retrieve the status
 * @param status the buffer to store the status
 * @param max_status_len max length of status to copy into provided buffer 
 * @return int 0 if the status is successfully retrieved, otherwise -1
 */ 
int get_http_response_status(http_resp response, char *status, size_t max_status_len);


/**
 * @brief Get the http response content size (bytes)
 * 
 * @param response the response from which to retrieve the content size
 * @param content_size reference to store the http response content size
 * @return int 0 if the content size is successfully retrieved, otherwise -1
 */
int get_http_response_content_size(http_resp response, off_t *content_size);


/**
 * @brief Get the http response resssource fd.
 * 
 * @param response the response from which to retrieve the ressource fd
 * @param ressource_fd reference to store the ressource fd
 * @return int 0 if the fd was retrieved successfully, otherwise -1
 */
int get_http_response_ressource_fd(http_resp response, int *ressource_fd);


/**
 * @brief Get the http response headers
 * 
 * @param response the response from which to retrieve the headers
 * @param headers the buffer to store the headers
 * @param max_body_len max length of headers to copy into provided buffer
 * @return int 0 if the header is successfully retrieved, otherwise -1
 */
int get_http_response_headers(http_resp response, char *headers, size_t max_body_len);

/**
 * @brief Get the type of http response (simple, full)
 * 
 * @param response the response from which to retrieve the type
 * @param type pointer to where the type should be stored
 * @return int 0 if type is successfully retrieved, otherwise -1
 */
int get_http_response_type(http_resp response, int *type);


/**
 * @brief Get the http response status code (200, 404, etc.)
 * 
 * @param response the response from which to retrieve the status code.
 * @param status_code reference to store the status_code result.
 * @return int 0 if status_code was retrieved, otherwise -1
 */
int get_http_response_status_code(http_resp response, int *status_code);


#endif