/*
 * Client can be launched in two modes
 * client       => Prompts user for input
 *                  ./client <server_ip> <server_port>
 * client_eval  => used for scale testing, sends message with default string
 *                takes number of messages as 3rd arguement.
 *                  ./client_eval <server_ip> <client_ip> <num_of_msgs>
 */
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include "common.h"

/*Function headers*/
static int connectToServer(char *argv[]);
static inline double subtractTime(struct timeval *x, struct timeval *y);

/*
 * Input: command line arguments
 * Output: client socketfd
 * Opens connection to server on ip and port from command line
 */
static int
connectToServer(char *argv[])
{
    struct sockaddr_in servAddr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct timeval tv;

#ifdef EVAL_MODE
    printf("Client running in evaluation mode\n");
#endif

    servAddr.sin_family = AF_INET; 
    servAddr.sin_addr.s_addr = inet_addr(argv[1]); 
    servAddr.sin_port = htons(atoi(argv[2]));

    if (connect(sockfd, (struct sockaddr *)&servAddr, sizeof(servAddr)) != 0)
    { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 

    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

    return sockfd;

}

/*
 * Input: time before send, time after recv 
 * Output: time difference in seconds
 * Helper routine to calculate difference between two time values
 */
static inline double
subtractTime(struct timeval *x, struct timeval *y)  
{  
        double diff = x->tv_sec - y->tv_sec;  
            diff += (x->tv_usec - y->tv_usec)/1000000.0;  

                return diff;  
}

/*
 * Main function
 */     
int
main(int argc, char *argv[])
{

    int numbytes;
    char recvBuff[MAX_MSG_SZ];
    struct sockaddr_in serv_addr; 
    struct hostent *server;
    int rv;
    int sum;
    static int id = 1;
    int sockfd = -1;
    struct timeval sendTime, recvTime, timeDiff;
    int timeTaken = 0;
    int avg_time = 0;
    double diff = 0;
    double avg_time2 = 0;

    int ret=0;

    int numMsgs = 0;
    Message msg;

#ifdef EVAL_MODE    
    if(argc != 4)
    {
        printf("\nUsage: %s server_address port number_of_msgs\n", argv[0]);
        return 0;
    }

    numMsgs = atoi(argv[3]);
#else
    if(argc != 3)
    {
        printf("\nUsage: %s server_address port number_of_msgs\n", argv[0]);
        return 0;
    }
#endif

    memset(&msg, '0', sizeof(msg));
    memset(recvBuff, '0',sizeof(recvBuff));

    /*Open connection to server on ip and port from commandline*/
    sockfd = connectToServer(argv);

#ifdef EVAL_MODE        
    /*Test code to evaluate scaling performance*/
    strncpy(msg.data, "Hello world", 20);
    while(numMsgs>0)
    {
        numMsgs--;
#else
    /*Prompts user for input*/
    printf("Enter your message. Keep entering message. Press Ctrl+c when done entering\n");
    while (fgets(msg.data,MAX_MSG_SZ-1,stdin))
    {
        msg.data[strlen(msg.data)-1] = '\0';
#endif        
        msg.id = htonl(id);

        /*reset time stamps*/
        sendTime.tv_sec = 0;
        sendTime.tv_usec = 0;
        recvTime.tv_sec =0;
        recvTime.tv_usec =0;

        /*timestamp before sending the msg*/
        gettimeofday(&sendTime, NULL);

        if (-1 == send(sockfd, (void *)&msg, sizeof(msg),MSG_NOSIGNAL))
        {
                printf("Server disconnected\n");
                break;
        }
 
        /*receive reply from server*/
        numbytes = recv(sockfd,recvBuff,sizeof(recvBuff)-1,0);
        recvBuff[numbytes] = '\0';

#ifndef EVAL_MODE
        printf("Reply from server: %s\n",recvBuff);
#endif
        /*timestamp after receiving the msg*/
        gettimeofday(&recvTime, NULL);

        /*time interval between sending msg and receiving reply*/
        diff = subtractTime(&recvTime, &sendTime);

        /*calculate rolling average. Not ideal but will do*/
        avg_time2 = (avg_time2*(id-1) + diff)/id;

        id++;
    }

    printf("Avg round trip time(rtt):%lf\n", avg_time2);
    printf("Closing the client\n");
    return 0;
}
