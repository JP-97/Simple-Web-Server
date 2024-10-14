#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <cmocka.h>
#include "http_private.h"


static void test_init_http_request(void **state) {
    (void) state;
}


static void test_destroy_http_request(void **state) {
    (void) state;
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_init_http_request),
        cmocka_unit_test(test_destroy_http_request),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
