#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "command_line_private.h"

#define UNUSED (void)

typedef struct {
    int argc;
    char *argv[5];
    struct cli test_cli;
} command_line_t;

static int setup(void **state){
    command_line_t *cmd_line = malloc(sizeof(command_line_t)); 
    *state = cmd_line;
    return 0;
}
 
static int teardown(void **state) {
    free(*state);
    return 0;
}

// Mocks

int __wrap_access(const char * __name, int __type){
    check_expected_ptr(__name);
    check_expected(__type);

    return mock_type(int);
}


static void test_invalid_num_arguments(void **state) {
    command_line_t *cmd_line = *state;

    cmd_line->argc = 1;
    cmd_line->argv[0] = "blah";
    
    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_port_range_low(void **state) {
    char port[10];
    command_line_t *cmd_line = *state;

    sprintf(port, "%d", PORT_MIN-1);

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = port;
    cmd_line->argv[2] = "good_ressource";

    // Note CLI validation is done left-to-right, so don't
    // need to mock out "access" calls for server root

    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_port_range_high(void **state) {
    char port[10];
    command_line_t *cmd_line = *state;

    sprintf(port, "%d", PORT_MAX+1);

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = port;
    
    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_non_integer_port(void **state) {
    command_line_t *cmd_line = *state;

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = "bad port!";
    
    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_non_existent_server_root(void **state){
    command_line_t *cmd_line = *state;
    char port[10];
    sprintf(port, "%d", PORT_MIN+1);

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = port;
    cmd_line->argv[2] = "non_existent_ressource";

    // Set up mock to reflect bad ressource
    // Expectation is the we check existence for file prior to other checks
    // So don't need to stage multiple invocations in this case
    will_return(__wrap_access, -1);
    expect_string(__wrap_access, __name, "non_existent_ressource");
    expect_value(__wrap_access, __type, F_OK);

    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_server_root_thats_not_readable(void **state){
    command_line_t *cmd_line = *state;
    char port[10];
    sprintf(port, "%d", PORT_MIN+1);

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = port;
    cmd_line->argv[2] = "unreadable_server_root";

    will_return(__wrap_access, 0); // dir exists
    will_return(__wrap_access, -1); // dir is not readable
    expect_string_count(__wrap_access, __name, "unreadable_server_root", 2);
    expect_value(__wrap_access, __type, F_OK);
    expect_value(__wrap_access, __type, R_OK);


    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_server_root_thats_too_long(void **state){
    command_line_t *cmd_line = *state;
    char port[10];
    char long_server_root[MAX_SERVER_ROOT_LEN+10];
    memset(long_server_root, 10, sizeof(long_server_root));
    sprintf(port, "%d", PORT_MIN+1);

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = port;
    cmd_line->argv[2] = long_server_root;

    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_false(result);
}

static void test_valid_cli_input(void **state) {
    char port[10];
    command_line_t *cmd_line = *state;

    sprintf(port, "%d", PORT_MIN+1);

    cmd_line->argc = MIN_ARGUMENTS;
    cmd_line->argv[0] = "sws";
    cmd_line->argv[1] = port;
    cmd_line->argv[2] = "valid_server_root";

    // Set up mock to reflect a good server root
    will_return_always(__wrap_access, 0);
    expect_string_count(__wrap_access, __name, "valid_server_root", -1); // Check for exists, readable, etc.
    expect_any_always(__wrap_access, __type);
    
    bool result = parse_cli(cmd_line->argc, cmd_line->argv, &(cmd_line->test_cli));
    
    assert_true(result);
    assert_int_equal(cmd_line->test_cli.port, PORT_MIN+1);
    assert_string_equal(cmd_line->test_cli.server_root, "valid_server_root");
}



int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_invalid_num_arguments),
        cmocka_unit_test(test_port_range_low),
        cmocka_unit_test(test_port_range_high),
        cmocka_unit_test(test_non_integer_port),
        cmocka_unit_test(test_non_existent_server_root),
        cmocka_unit_test(test_server_root_thats_too_long),
        cmocka_unit_test(test_server_root_thats_not_readable),
        cmocka_unit_test(test_valid_cli_input),
    };

    return cmocka_run_group_tests(tests, setup, teardown);
}