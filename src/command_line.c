#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command_line.h"

static void _print_help();

void parse_cli(int argc, char *argv[], struct cli **result){
    int port_num;

    if(!result){
        printf("ERROR: invalid reference provided as input to arg parser...\n");
        return;
    }

    *result = NULL;

    if(argc < 2){
        _print_help();
        return;
    }
    else if(argc == 2 && (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)){
        _print_help();
        return;
    }

    // Validate port number
    port_num = strtol(argv[1], NULL, 10);

    if(port_num == 0){
        // non-integer provided for port
        printf("Port must be able to cast to an integer!...\n\n");
        _print_help();
        return;
    }
    else if(port_num < PORT_MIN || port_num > PORT_MAX){
        _print_help();
        return;
    }

    struct cli *resp = (struct cli *)calloc(1, sizeof(struct cli));
    
    if(!resp){
        printf("ERROR: Failed to allocate memory for parsed CLI arguments...\n");
        return;
    }

    resp->port = port_num;
    *result = resp;
    return;
}

static void _print_help(){
    printf("Usage: sws PORT\n\n");
    printf("PORT must be in range %d to %d and represents the port that your server will run on.\n", PORT_MIN, PORT_MAX);
    return;
}