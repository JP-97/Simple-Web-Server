#ifndef _CMD_LINE
#define _CMD_LINE

#include <stdbool.h>

#define MIN_ARGUMENTS 3
#define PORT_MIN 1500
#define PORT_MAX 10000
#define MAX_SERVER_ROOT_LEN 500

extern char server_root_location[MAX_SERVER_ROOT_LEN];

struct cli {
    int port; /**< Port in which the server will run locally. */
    char server_root[MAX_SERVER_ROOT_LEN]; /**< Relative path to where server ressources are located.*/
};


/**
 * @brief Parse and validate CLI options provided to server application
 *
 * @param argc integer containing number of cli arguments
 * @param argv array of char pointers, where each pointer points to cli argument as a string
 * @param result pointer to pointer of type struct cli in which results will be stored. NULL if error occured.
*/
bool parse_cli(int argc, char *argv[], struct cli *result);


#endif