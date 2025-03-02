#ifndef _HTTP_PRIVATE
#define _HTTP_PRIVATE

#include <sys/stat.h>
#include "http.h"

#define NUM_RECOGNIZED_EXT_MAPPINGS 5

struct _http_req{
    http_method method;
    char URI[MAX_URI_LEN];
    char version[MAX_VER_LEN];

    // Internal use only
    char _ressource_name[MAX_RESSOURCE_LEN];
    char _ressource_location[MAX_URL_LEN];
    char _ressource_abs_path[MAX_SERVER_ROOT_LEN + MAX_URI_LEN];
};

struct _http_resp {
    char status[MAX_RESP_STATUS_LEN];
    char headers[MAX_RESP_HEADERS_LEN];
    int  ressource_fd;
    http_response_type response_type;

    // Internal use only
    off_t            _content_len;
    char             _content_type[MAX_RES_TYPE_LEN];
    http_return_code _return_code;
};

typedef struct ext_map {
    char *extension;
    char *content_type;
} ext_map;

void alloc_http_request(http_req *result);

#endif