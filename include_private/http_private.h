#ifndef _HTTP_PRIVATE
#define _HTTP_PRIVATE

#include "http.h"

struct _http_req{
    http_method method;
    char URI[MAX_URI_LEN];
    char version[MAX_VER_LEN];
    char _ressource_name[MAX_RESSOURCE_LEN];
    char _ressource_location[MAX_URL_LEN];
};

struct _http_resp {
    char status[MAX_RESP_STATUS_LEN];
    char headers[MAX_RESP_HEADERS_LEN];
    char body[MAX_RESP_BODY_LEN];
    http_response_type response_type;
    
    // Internal use only
    http_return_code _return_code;
};

void alloc_http_request(http_req *result);

#endif