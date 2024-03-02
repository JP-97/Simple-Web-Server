#ifndef CMD_LINE
#define CMD_LINE

# define MIN_ARGUMENTS 2
# define PORT_MIN 1500
# define PORT_MAX 10000

struct cli {
    int port;
};


/**
 * Parse and validate CLI options provided to server application
 *
 * @param argc integer containing number of cli arguments
 * @param argv array of char pointers, where each pointer points to cli argument as a string
 * @param result pointer to pointer of type struct cli in which results will be stored. NULL if error occured.
*/
void parse_cli(int argc, char *argv[], struct cli **result);


#endif