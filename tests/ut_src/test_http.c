#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <assert.h>
#include "http_private.h"
#include "command_line_private.h"


typedef struct _http_test_t {
    http_req request;
} http_test_t;

static int setup_standard_request(void **state){
    http_test_t *test_data = calloc(1, sizeof(http_test_t)); 
    *state = test_data;
    test_data->request = alloc_http_request();

    // Set up some defaults for request
    test_data->request->method = GET;
    strcpy(test_data->request->URI, "/");
    strcpy(test_data->request->version, "1.0");
    strcpy(test_data->request->_ressource_name, "/index.html");

    return 0;
}
 
static int destroy_standard_request(void **state) {
    http_test_t *test_data = (http_test_t*) *state;
    destroy_http_request(&(test_data->request));
    free(*state);
    *state = NULL;
    return 0;
}

/**
 * @brief Currently, requested ressources are prechecked early on in processing
 *        pipeline to ensure it both exists and is readable. This helper will
 *        apply the appropriate mocks for provided request to ensure it passes
 *        the ressource validity checks. 
 * 
 * @param request request containing ressource that needs to be mocked.
 */
static void mock_valid_ressource(http_req request){
    assert(request != NULL);
    
    // Ressource exists in filesystem
    expect_string(__wrap_access, __name, request->_ressource_name);
    expect_value(__wrap_access, __type, F_OK);
    will_return(__wrap_access, 0);

    // Ressource is readable
    expect_string(__wrap_access, __name, request->_ressource_name);
    expect_value(__wrap_access, __type, R_OK);
    will_return(__wrap_access, 0);
}


static void test_get_http_response_from_request_bad_request_version(void **state){    
    http_test_t *test_data = (http_test_t*) *state;
    strcpy(test_data->request->version, "0.0"); // 0.0 is invalid request version

    mock_valid_ressource(test_data->request);

    http_resp response = get_http_response_from_request(test_data->request);

    // Version is not a valid version - we should get a "full" response 
    // with bad request as the code
    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, BAD_REQUEST);
}

static void test_get_http_response_from_request_unsupported_request_version(void **state){
    http_test_t *test_data = (http_test_t*) *state;
    strcpy(test_data->request->version, "3.0"); // Server only supports versions <=1.1

    mock_valid_ressource(test_data->request);

    http_resp response = get_http_response_from_request(test_data->request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, UNSUPPORTED_VER);
}

static void test_get_http_response_from_request_valid_simple_request(void **state){
    int ressource_fd = 5;
    int content_len = 1000;

    http_test_t *test_data = (http_test_t*) *state;
    strcpy(test_data->request->version, ""); // Version not provided as part of simple request

    mock_valid_ressource(test_data->request);

    // Mock ressource size
    expect_value(__wrap_fstat, __fd, ressource_fd);
    will_return(__wrap_fstat, content_len);
    will_return(__wrap_fstat, 0);

    // Mock ressource fd creation
    expect_string(__wrap_open, __file, "/index.html");
    expect_value(__wrap_open, __oflag, O_RDONLY);
    will_return(__wrap_open, ressource_fd);

    http_resp response = get_http_response_from_request(test_data->request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, SIMPLE);
    assert_int_equal(response->_return_code, 200);
}

static void test_get_http_response_from_request_valid_full_request(void **state){
    int ressource_fd = 5;
    int content_len = 1000;
    
    http_test_t *test_data = (http_test_t*) *state;

    mock_valid_ressource(test_data->request);

    // Mock ressource size
    expect_value(__wrap_fstat, __fd, ressource_fd);
    will_return(__wrap_fstat, content_len);
    will_return(__wrap_fstat, 0);

    // Mock ressource fd creation
    expect_string(__wrap_open, __file, test_data->request->_ressource_name);
    expect_value(__wrap_open, __oflag, O_RDONLY);
    will_return(__wrap_open, ressource_fd);

    printf("%s", test_data->request->version);

    http_resp response = get_http_response_from_request(test_data->request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, OK);
    assert_int_equal(response->_content_len, content_len);
}

static void test_get_http_response_from_request_valid_post_request(void **state){
    http_req dummy_request;
    alloc_http_request(&dummy_request);
    
    // Set up dummy requst
    dummy_request->method = POST;
    strncpy(dummy_request->URI, "/", MAX_URI_LEN+1);
    strncpy(dummy_request->version, "1.0", MAX_VER_LEN+1); // http 1.0 request (full)

    http_resp response = get_http_response_from_request(dummy_request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, 200);
}

static void test_get_http_response_from_request_valid_head_request(void **state){
    http_req dummy_request;
    alloc_http_request(&dummy_request);
    
    // Set up dummy requst
    dummy_request->method = HEAD;
    strncpy(dummy_request->URI, "/", MAX_URI_LEN+1);
    strncpy(dummy_request->version, "1.0", MAX_VER_LEN+1); // http 1.0 request (full)

    http_resp response = get_http_response_from_request(dummy_request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, 200);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_get_http_response_from_request_bad_request_version, setup_standard_request, destroy_standard_request),
        cmocka_unit_test_setup_teardown(test_get_http_response_from_request_valid_simple_request, setup_standard_request, destroy_standard_request),
        cmocka_unit_test_setup_teardown(test_get_http_response_from_request_unsupported_request_version,setup_standard_request, destroy_standard_request),
        cmocka_unit_test_setup_teardown(test_get_http_response_from_request_valid_full_request, setup_standard_request, destroy_standard_request),
        // cmocka_unit_test_setup_teardown(test_get_http_response_from_request_valid_post_request, setup_standard_request, destroy_standard_request),
        // cmocka_unit_test_setup_teardown(test_get_http_response_from_request_valid_head_request, setup_standard_request, destroy_standard_request),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
