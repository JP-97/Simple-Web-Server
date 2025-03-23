#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "command_line_private.h"
#include "log.h"

static void _print_help();

typedef bool (*cli_validation_func)(char *, struct cli *);

char server_root_location[MAX_SERVER_ROOT_LEN];
log_level user_provided_log_level = DEFAULT;

bool parse_cli(int argc, char *argv[], struct cli *result){
    if(!result){
        printf("ERROR: invalid reference provided as input to arg parser...\n");
        return false;
    }

    if(argc < MIN_ARGUMENTS){
        printf("ERROR: invalid number of input arguments provided...\n");
        _print_help();
        return false;
    }

    else if(argc == 2 && (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)){
        _print_help();
        return false;
    }

    else if(argc > MIN_ARGUMENTS){
        for(int i = MIN_ARGUMENTS; i < argc; i++){
            if(strcmp(argv[i], "-v") == 0){
                user_provided_log_level = DEBUG;
            }
        }
    }

    // Note: Important that validation functions are ordered the same as
    // how they appear in the CLI prompt, otherwise argument mapping will break 
    cli_validation_func validation_funcs[] = {validate_port_num, validate_server_root};

    for(int i=0; i<(sizeof(validation_funcs) / sizeof(validation_funcs[0])); i++){
        bool argument_valid = validation_funcs[i](argv[i+1], result);
        
        if(!argument_valid){
            _print_help();
            return false;
        }
    }
    return true;
}


bool validate_port_num(char *port_num_to_validate, struct cli *result){
    int port_num_tmp;

    if(!result){
        printf("ERROR: Internal error processing port num!...\n");
        return false;
    }

    port_num_tmp = strtol(port_num_to_validate, NULL, 10);

    if(port_num_tmp == 0){
        // non-integer provided for port
        printf("ERROR: Port must be able to cast to an integer!...\n\n");
        return false;

    }
    else if(port_num_tmp < PORT_MIN || port_num_tmp > PORT_MAX){
        printf("ERROR: Provided port is outside of allowable port range!...\n");
        return false;
    }

    result->port = port_num_tmp;
    return true;
}


bool validate_server_root(char *server_root_to_validate, struct cli *result){
    char server_root_tmp[MAX_SERVER_ROOT_LEN];
    int was_copied;

    if(!result){
        printf("ERROR: Internal error processing server name!...\n");
        return false;
    }

    was_copied = snprintf(server_root_tmp, MAX_SERVER_ROOT_LEN, "%s", server_root_to_validate);

    if(was_copied == -1 || was_copied >= MAX_SERVER_ROOT_LEN){
        printf("ERROR: Failed to process provided server root!...\n");
        return false;
    }

    else if(access(server_root_tmp, F_OK) != 0){
        printf("ERROR: Provided server root (%s) is not accessible!...\n", server_root_to_validate);
        return false;
    }

    else if(access(server_root_tmp, R_OK) != 0 || access(server_root_tmp, X_OK) != 0){
        printf("ERROR: Provided server root (%s) doesn't have the right permissions!...\n", server_root_to_validate);
        return false;
    }

    strncpy(result->server_root, server_root_tmp, MAX_SERVER_ROOT_LEN);
    strcpy(server_root_location, result->server_root); // Set the extern server_root_location variable

    return true;
}


static void _print_help(){
    printf("Usage: sws PORT SERVER_ROOT [-v]\n\n");
    printf("\nPORT must be in range %d to %d and represents the port that your server will run on.\n", PORT_MIN, PORT_MAX);
    printf("\nSERVER_ROOT must be a valid path on host machine which doesn't exceed %d characters.\n", MAX_SERVER_ROOT_LEN-1);
    printf("\n-v to run server with added verbosity");
    printf("\n");
    return;
}