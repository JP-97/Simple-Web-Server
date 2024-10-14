#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <semaphore.h>
#include "bbuf_private.h"

#define UNUSED (void)


extern void* _test_malloc(const size_t size, const char* file, const int line);
extern void _test_free(void* const ptr, const char* file, const int line);

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)


typedef struct _test_context_t {
    bbuf_t buff;
} test_context_t;


static int group_setup(void **state){
    *state = malloc(sizeof(test_context_t));
    return 0;
}
 
static int group_teardown(void **state) {
    free(*state);
    return 0;
}

static int init_empty_bbuf(void **state){
    test_context_t *ctx = (test_context_t *) *state;
    ctx->buff = bbuf_init();

    assert_int_equal(get_bbuf_items(ctx->buff), 0);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE);

    return 0;
}

static int destroy_bbuf(void **state){
    test_context_t *ctx = (test_context_t *) *state;

    bbuf_destroy(ctx->buff);
    return 0;
}


static void test_bbuf_insert_new_buffer(void **state){
    test_context_t *ctx = (test_context_t *) *state;
    int dummy_fd = 50;
    int return_code;


    return_code = bbuf_insert(ctx->buff, dummy_fd);
    assert_int_equal(return_code, 0);

    // Make sure that newly added item is advertised
    assert_int_equal(get_bbuf_items(ctx->buff), 1);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE-1);
}


static void test_bbuf_several_sequential_inserts(void **state){
    test_context_t *ctx = (test_context_t *) *state;
    int dummy_fd = 50;
    int num_inserts = 5;
    
    for(int i=0; i<num_inserts; i++){
        int return_code = bbuf_insert(ctx->buff, dummy_fd+i);
        assert_int_equal(return_code, 0);
    }

    // Make sure that newly added items are advertised
    assert_int_equal(get_bbuf_items(ctx->buff), 5);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE-5);
}

static void test_bbuf_insert_null_buff_reference(void **state){
    UNUSED state;

    assert_int_equal(bbuf_insert(NULL, 100), -1);
}


static void test_bbuf_remove(void **state){
    test_context_t *ctx = (test_context_t *) *state;
    int dummy_fd = 50;
    int popped_fd;
    
    bbuf_insert(ctx->buff, dummy_fd); 

    assert_int_equal(get_bbuf_items(ctx->buff), 1);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE-1);

    bbuf_remove(ctx->buff, &popped_fd); 

    // Make sure that bbuf size reflects popped item
    assert_int_equal(get_bbuf_items(ctx->buff), 0);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE);
    assert_int_equal(popped_fd, dummy_fd);
}


static void test_bbuf_remove_multiple_removes(void **state){
    test_context_t *ctx = (test_context_t *) *state;
    int num_operations = 5;
    int popped_fd;
    
    // Put some fds in the buffer
    for(int i = 0; i<num_operations; i++){
        bbuf_insert(ctx->buff, i+10);
    }

    assert_int_equal(get_bbuf_items(ctx->buff), num_operations);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE-num_operations);

    // Make sure the same fds are removed in same order they were added
    for(int i = 0; i<num_operations; i++){
        bbuf_remove(ctx->buff, &popped_fd);
        assert_int_equal(popped_fd, i+10);
    }

    assert_int_equal(get_bbuf_items(ctx->buff), 0);
    assert_int_equal(get_bbuf_slots(ctx->buff), BBUF_SIZE);
}


static void test_bbuf_remove_null_buff_reference(void **state){
    UNUSED state;
    int blah;
    assert_int_equal(bbuf_remove(NULL, &blah), -1);
}


static void test_bbuf_no_result_value_provided(void **state){
    test_context_t *ctx = (test_context_t *) *state;

    bbuf_insert(ctx->buff, 10);
    assert_int_equal(get_bbuf_items(ctx->buff), 1);
    assert_int_equal(bbuf_remove(ctx->buff, NULL), 0);
    assert_int_equal(get_bbuf_items(ctx->buff), 0);
}


static void test_get_bbuf_max_size(void **state){
    test_context_t *ctx = (test_context_t *) *state;
    assert_int_equal(get_bbuf_max_size(ctx->buff), BBUF_SIZE);
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup_teardown(test_bbuf_insert_new_buffer, init_empty_bbuf, destroy_bbuf),
        cmocka_unit_test_setup_teardown(test_bbuf_several_sequential_inserts, init_empty_bbuf, destroy_bbuf),
        cmocka_unit_test(test_bbuf_insert_null_buff_reference),
        cmocka_unit_test_setup_teardown(test_bbuf_remove, init_empty_bbuf, destroy_bbuf),
        cmocka_unit_test_setup_teardown(test_bbuf_remove_multiple_removes, init_empty_bbuf, destroy_bbuf),
        cmocka_unit_test(test_bbuf_remove_null_buff_reference),
        cmocka_unit_test_setup_teardown(test_bbuf_no_result_value_provided, init_empty_bbuf, destroy_bbuf),
        cmocka_unit_test_setup_teardown(test_get_bbuf_max_size, init_empty_bbuf, destroy_bbuf)
    };

    return cmocka_run_group_tests(tests, group_setup, group_teardown);
}