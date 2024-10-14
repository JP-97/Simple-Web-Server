#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


static int test_setup(void **state){
    return 0;
}


static int test_teardown(void **state) {
    return 0;
}


// int main(void) {
//     // const struct CMUnitTest tests[] = {
//     //     cmocka_unit_test(test_parse_request_null_request),
//     // };

//     return cmocka_run_group_tests(tests, test_setup, test_teardown);
// }