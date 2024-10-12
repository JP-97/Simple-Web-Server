#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include "command_line.h"

#define UNUSED (void)

typedef struct {
    int argc;
    char *argv[5];
    struct cli *test_cli;
} command_line_t;

static int setup(void **state){
    // Disable stdout
    // FILE *original_stdout = stdout;
    // stdout = fopen("/dev/null", "w");
    // 
    // *state = original_stdout;

    command_line_t *cmd_line = malloc(sizeof(command_line_t)); 
    *state = cmd_line;

    return 0;
}
 
static int teardown(void **state) {
    // Restore stdout back to original fd
    // stdout = *state;

    free(*state);
    return 0;
}


static void test_invalid_num_arguments(void **state) {
    command_line_t *cmd_line = *state;

    cmd_line->argc = 1;
    cmd_line->argv[0] = "blah";
    
    parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    // test_cli shouldn't get populated if input is invalid
    assert_null(cmd_line->test_cli);
}

static void test_port_range_low(void **state) {
    char port[10];
    command_line_t *cmd_line = *state;

    sprintf(port, "%d", PORT_MIN-1);

    cmd_line->argc = 2;
    cmd_line->argv[0] = "sws.c";
    cmd_line->argv[1] = port;
    
    parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    // test_cli shouldn't get populated if input is invalid
    assert_null(cmd_line->test_cli);
}

static void test_port_range_high(void **state) {
    char port[10];
    command_line_t *cmd_line = *state;

    sprintf(port, "%d", PORT_MAX+1);

    cmd_line->argc = 2;
    cmd_line->argv[0] = "sws.c";
    cmd_line->argv[1] = port;
    
    parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    // test_cli shouldn't get populated if input is invalid
    assert_null(cmd_line->test_cli);
}

static void test_non_integer_port(void **state) {
    command_line_t *cmd_line = *state;

    cmd_line->argc = 2;
    cmd_line->argv[0] = "sws.c";
    cmd_line->argv[1] = "bad port!";
    
    parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    // test_cli shouldn't get populated if input is invalid
    assert_null(cmd_line->test_cli);
}

static void test_valid_port(void **state) {
    char port[10];
    command_line_t *cmd_line = *state;

    sprintf(port, "%d", PORT_MIN+1);

    cmd_line->argc = 2;
    cmd_line->argv[0] = "sws.c";
    cmd_line->argv[1] = port;
    
    parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_non_null(cmd_line->test_cli);
    assert_int_equal(cmd_line->test_cli->port, PORT_MIN+1);
}



int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_invalid_num_arguments),
        cmocka_unit_test(test_port_range_low),
        cmocka_unit_test(test_port_range_high),
        cmocka_unit_test(test_non_integer_port),
        cmocka_unit_test(test_valid_port),
    };

    return cmocka_run_group_tests(tests, setup, teardown);
}