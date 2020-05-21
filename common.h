#ifndef __COMMON_H__
#define __COMMON_H__

/*
 * Maximum size of message exchanged
 * between client and server.
 * Bigger user inputs are split and sent
 * as separate messages.
 */
#define MAX_MSG_SZ 1024

/* 
 * Message structure to be exchanged
 * client and server
 */
#pragma pack(1)
typedef struct {

    int id;
    char data[MAX_MSG_SZ];
} Message;
#pragma pack(0)

#endif
