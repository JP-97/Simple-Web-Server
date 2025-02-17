#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdio.h>
#include <string.h>
#include "http_private.h"


static void test_init_http_request(void **state) {
    (void) state;
}


static void test_destroy_http_request(void **state) {
    (void) state;
}

static void test_get_http_response_from_request_bad_request_version(void **state){
    http_req dummy_request;
    alloc_http_request(&dummy_request);
    
    // Set up dummy request
    dummy_request->method = GET;
    strncpy(dummy_request->URI, "/", MAX_URI_LEN+1);
    strncpy(dummy_request->version, "0.0", MAX_VER_LEN+1); // version should never start with a 0

    http_resp response = get_http_response_from_request(dummy_request);

    // Version is not a valid version - we should get a "full" response 
    // with bad request as the code
    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, BAD_REQUEST);
}

static void test_get_http_response_from_request_unsupported_request_version(void **state){
    http_req dummy_request;
    alloc_http_request(&dummy_request);
    
    // Set up dummy request
    dummy_request->method = GET;
    strncpy(dummy_request->URI, "/", MAX_URI_LEN+1);
    strncpy(dummy_request->version, "3.0", MAX_VER_LEN+1);

    http_resp response = get_http_response_from_request(dummy_request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, UNSUPPORTED_VER);
}

static void test_get_http_response_from_request_valid_simple_request(void **state){
    http_req dummy_request;
    alloc_http_request(&dummy_request);
    
    // Set up dummy simple request
    // NOTE: Simple request doesn't have a request version
    dummy_request->method = GET;
    strncpy(dummy_request->URI, "/", MAX_URI_LEN+1);

    http_resp response = get_http_response_from_request(dummy_request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, SIMPLE);
    assert_int_equal(response->_return_code, 200);
}

static void test_get_http_response_from_request_valid_full_request(void **state){
    http_req dummy_request;
    alloc_http_request(&dummy_request);
    
    // Set up dummy requst
    dummy_request->method = GET;
    strncpy(dummy_request->URI, "/", MAX_URI_LEN+1);
    strncpy(dummy_request->version, "1.0", MAX_VER_LEN+1); // http 1.0 request (full)

    http_resp response = get_http_response_from_request(dummy_request);

    assert_ptr_not_equal(response, NULL);
    assert_int_equal(response->response_type, FULL);
    assert_int_equal(response->_return_code, 200);
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
        cmocka_unit_test(test_init_http_request),
        cmocka_unit_test(test_destroy_http_request),
        cmocka_unit_test(test_get_http_response_from_request_bad_request_version),
        cmocka_unit_test(test_get_http_response_from_request_valid_simple_request),
        cmocka_unit_test(test_get_http_response_from_request_unsupported_request_version),
        cmocka_unit_test(test_get_http_response_from_request_valid_full_request),
        cmocka_unit_test(test_get_http_response_from_request_valid_post_request),
        cmocka_unit_test(test_get_http_response_from_request_valid_head_request),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
