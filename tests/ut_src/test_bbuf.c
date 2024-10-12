#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <semaphore.h>
#include "bbuf.h"

#define UNUSED (void)
#define malloc(size) custom_malloc(size)

typedef struct {
    bbuf_t *buff;
} test_context_t;


static int test_setup(void **state){
    return 0;
}
 
static int test_teardown(void **state) {
    return 0;
}

static void test_bbuf_init_successful_init(void **state){
    UNUSED state;
    
    bbuf_t *dummy_buff;
    int rc = bbuf_init(&dummy_buff);
    assert_int_equal(rc, 0);
    assert_int_equal(get_bbuf_size(dummy_buff), BBUF_SIZE);
    assert_int_equal(get_bbuf_slots(dummy_buff), 25);
    assert_int_equal(get_bbuf_items(dummy_buff), 0);
}

static void test_bbuf_init_null_buff(void **state){
    UNUSED state;
    
    int rc = bbuf_init(NULL);
    assert_int_equal(rc, -1);
}

static void test_bbuf_successful_free(void **state){
    bbuf_t *dummy_buff;
    int rc;
    UNUSED bbuf_init(&dummy_buff);
    assert_int_not_equal(sizeof(*dummy_buff), 0); // Make sure we have something to free...
   
    rc = bbuff_free(&dummy_buff);
   
    assert_int_equal(rc, 0);
    assert_null(dummy_buff);
}

static void test_bbuf_insert_new_buffer(void **state){
    UNUSED state;

    int dummy_fd = 50;
    bbuf_t *dummy_buff;
    
    UNUSED bbuf_init(&dummy_buff);

    assert_int_equal(get_bbuf_items(dummy_buff), 0);
    assert_int_equal(get_bbuf_slots(dummy_buff), BBUF_SIZE);

    bbuf_insert(dummy_buff, dummy_fd); 

    // Make sure that newly added item is advertised
    assert_int_equal(get_bbuf_items(dummy_buff), 1);
    assert_int_equal(get_bbuf_slots(dummy_buff), BBUF_SIZE-1);

}

static void test_bbuf_several_consecutive_inserts(void **state){
    UNUSED state;

    int dummy_fd = 50;
    bbuf_t *dummy_buff;
    
    bbuf_init(&dummy_buff);

    assert_int_equal(get_bbuf_items(dummy_buff), 0);
    assert_int_equal(get_bbuf_slots(dummy_buff), BBUF_SIZE);

    // 5 consecutive inserts
    for(int i=0; i<5; i++){
        bbuf_insert(dummy_buff, dummy_fd);
    }

    // Make sure that newly added item is advertised
    assert_int_equal(get_bbuf_items(dummy_buff), 5);
    assert_int_equal(get_bbuf_slots(dummy_buff), BBUF_SIZE-5);

}

static void test_bbuf_insert_null_buffer(void **state){
    UNUSED state;

    assert_int_equal(bbuf_insert(NULL, 100), -1);
}


static void test_bbuf_remove(void **state){
    UNUSED state;

    int dummy_fd = 50, popped_fd;
    bbuf_t *dummy_buff;
    
    bbuf_init(&dummy_buff);
    bbuf_insert(dummy_buff, dummy_fd); 

    assert_int_equal(get_bbuf_items(dummy_buff), 1);
    assert_int_equal(get_bbuf_slots(dummy_buff), BBUF_SIZE-1);

    bbuf_remove(dummy_buff, &popped_fd); 

    // Make sure that bbuf size reflects popped item
    assert_int_equal(get_bbuf_items(dummy_buff), 0);
    assert_int_equal(get_bbuf_slots(dummy_buff), BBUF_SIZE);
    assert_int_equal(popped_fd, dummy_fd);
}

static void test_bbuf_remove_multiple_removes(void **state){
    UNUSED state;

    int num_operations = 5, popped_fd;
    bbuf_t *dummy_buff;
    
    bbuf_init(&dummy_buff);
    
    // Insert fds 10-15 in the buffer
    for(int i = 0; i<num_operations; i++){
        bbuf_insert(dummy_buff,i+10);
    }

    // Make sure the same fds are removed in same order they were added
    for(int i = 0; i<num_operations; i++){
        bbuf_remove(dummy_buff, &popped_fd);
        assert_int_equal(popped_fd, i+10);
    }
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_bbuf_init_successful_init),
        cmocka_unit_test(test_bbuf_init_null_buff),
        cmocka_unit_test(test_bbuf_successful_free),
        cmocka_unit_test(test_bbuf_insert_new_buffer),
        cmocka_unit_test(test_bbuf_several_consecutive_inserts),
        cmocka_unit_test(test_bbuf_insert_null_buffer),
        cmocka_unit_test(test_bbuf_remove),
        cmocka_unit_test(test_bbuf_remove_multiple_removes)
    };

    return cmocka_run_group_tests(tests, test_setup, test_teardown);
}