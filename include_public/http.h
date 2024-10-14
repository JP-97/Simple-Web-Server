/**
 * @file http.h
 * @author Josh Poirier
 * @date 4 Apr 2024
 * @brief File containing some helper APIs for handling HTTP requests.
 *
 */


#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>

#define MAX_METHOD_LEN      5
#define MAX_URI_LEN         50
#define MAX_VER_LEN         5

#define MAX_RESP_STATUS_LEN 60
#define MAX_RESP_HEADERS_LEN 200
#define MAX_RESP_BODY_LEN   8192


typedef struct _http_req *http_req;
typedef struct _http_resp *http_resp;


typedef enum _http_return_code {
    OK = 200,
    FILE_NOT_FOUND = 404

} http_return_code;


typedef enum _http_response_type {
    SIMPLE,
    FULL
} http_response_type;


/**
 * @brief Parse http request on client_fd.
 * 
 * @return http_req handler for request object.
*/
http_req init_http_request(int client_fd, size_t max_req_len);


/**
 * @brief Initializer for HTTP response
 * 
 * @return http_resp handler for response object.
 */
http_resp init_http_response();


/**
 * @brief Free all memory allocated to provided http_req object.
*/
void destroy_http_request(http_req request_to_destroy);


/**
 * @brief Free all memory allocated to provided http_req object.
*/
void destroy_http_response(http_resp response_to_destroy);


/**
 * @brief return the http method specified in the incoming request.
 * 
 * @param req Pointer to initialized http request
 * @param method pointer to location where to store result.
 */
int get_http_request_method(http_req req, char *method);


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
 * @param http_response pointer to http_resp pointer
 */
int get_http_response_from_request(http_req request_to_process, http_resp response);


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
 * @brief Get the http response body
 * 
 * @param response the response from which to retrieve the body
 * @param body buffer to store the body
 * @param max_body_len max length of body to copy into provided buffer
 * @return int 0 if the body is successfully retrieved, otherwise -1
 */
int get_http_response_body(http_resp response, char *body, size_t max_body_len);


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

#endif