#ifndef _CLI_PRIVATE
#define _CLI_PRIVATE

#include "command_line.h"

bool validate_server_root(char *server_root_to_validate, struct cli *result);
bool validate_port_num(char *port_num_to_validate, struct cli *result);

#endif