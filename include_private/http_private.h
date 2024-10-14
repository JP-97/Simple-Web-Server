#ifndef _HTTP_PRIVATE
#define _HTTP_PRIVATE

#include "http.h"

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


#endif