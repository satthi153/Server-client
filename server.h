#ifndef __SERVER_H__
#define __SERVER_H__

#include <stdio.h>
#include <stdlib.h>

/*MACROS*/
#define NUM_THREADS 5
#define CLIENT_TIMEOUT_SEC 180
#define MAX_CONTINUOUS_MSGS 100 
#define DEFAULT_PORT 55555

#define FILE_NAME_SZ 20

/*TODO: Multi-level tracing*/
#define LOG(log_level, str) \
    do { \
        if (log_level >= debug_level); \
            printf("%s",str); \
    } while (0)

enum LOG_LEVEL
{
    INFO,
    DEBUG,
    ERR
};

typedef struct
{
    uint32_t num_of_threads; /*number of worker threads*/
    uint32_t client_timeout; /*time to wait before closing client that is not sending data*/
    uint32_t max_continuous_msgs; /*maximum of continuous msgs processed from a client*/
    uint8_t debug_level; /*TODO: tracing options*/
    char file_name[FILE_NAME_SZ]; /*file to write to*/
    uint32_t port; /*port on which socket is opened*/
} SystemParams;

#endif

