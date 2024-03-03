#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command_line.h"

static void _print_help();

void parse_cli(int argc, char *argv[], struct cli **result){
    *result = NULL;
    
    int port_num;

    if(argc < 2){
        _print_help();
        return;
    }
    else if(argc == 2 && (argv[1] == "help" || argv[1] == "--help" || argv[1] == "-h")){
        _print_help();
        return;
    }

    // Validate port number
    port_num = atoi(argv[1]);

    if(port_num < PORT_MIN || port_num > PORT_MAX){
        _print_help();
        return;
    }

    struct cli *resp = (struct cli *)calloc(1, sizeof(struct cli));
    resp->port = port_num;

    *result = resp;

    return;
}

static void _print_help(){
    printf("Usage: sws PORT\n\n");
    printf("PORT must be in range %d to %d and represents the port that your server will run on.\n", PORT_MIN, PORT_MAX);
    return;
}