#ifndef __LOG
#define __LOG
#include <stdio.h>
#include <string.h>

#define MAX_LOG_MSG_LEN 1000

typedef enum _log_level {
    DEBUG,
    INFO,
    DEFAULT = INFO,
    WARNING,
    ERROR,
    MAX_LEVEL
} log_level;


#define LOG(level, ...)                                               \
        do {                                                          \
            extern log_level user_provided_log_level;                 \
            if(level < 0 || level >= MAX_LEVEL){                      \
                break;                                                \
            }                                                         \
            else if(level < user_provided_log_level){                 \
                break;                                                \
            }                                                         \
            char __log[30] = {0};                                     \
            char __usr_msg[MAX_LOG_MSG_LEN] = {0};                    \
            sprintf(__log, "%s:", #level);                                    \
            snprintf(__usr_msg, MAX_LOG_MSG_LEN, __VA_ARGS__);        \
            strcat(__log, __usr_msg);                                 \
            fprintf(level <= INFO ? stdout : stderr, "%s", __log);    \
        } while (0)

#endif