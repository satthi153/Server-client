/*
 * Multi-threaded Server
 * Highly configurable and scalable system 
 * capable of handling multiple clients sending continuous data
 */
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/wait.h>
#include <sched.h>

#include "myQueue.h"
#include "common.h"
#include "server.h"

/*Global stuff*/
SystemParams sysParams;
Q *clientQ;
FILE *fd;

/*For synchronization*/
pthread_mutex_t qMutex;
pthread_mutex_t fMutex;
pthread_cond_t cond;

/*Function headers*/
static void writeToFile(Message *msg);
static inline void setDefaultSystemParams(void);
static inline void usage(void);
static uint8_t parseArgs(int argc, char *argv[]);
static inline void printSystemParams(void);
static void processClient (int clientfd);
static void addClient(int clientfd);
static int getClient(void);
static void * connectionHandler();

/* Input: Msg to write to server file
 * Output: void
 * Called by worker threads to write to server file. fMutex used for synchronization
 */
static void
writeToFile(Message *msg)
{
    static total_msg_cnt;
    pthread_mutex_lock(&fMutex);

    msg->id = ntohl(msg->id);

    fprintf(fd,"msg_count: %d, msg_id: %d, msg: %s\n", total_msg_cnt, msg->id, msg->data);

    total_msg_cnt++;
    fflush(fd);

    pthread_mutex_unlock(&fMutex);
}

/*
 * Input: void
 * Output: void
 * sets system params to default values
 */
static inline void
setDefaultSystemParams(void) 
{
    sysParams.num_of_threads = NUM_THREADS;
    sysParams.client_timeout = CLIENT_TIMEOUT_SEC;
    sysParams.max_continuous_msgs = MAX_CONTINUOUS_MSGS;     
    sysParams.port = DEFAULT_PORT;
    snprintf(sysParams.file_name, FILE_NAME_SZ,"file") ;
}

/*
 * Input: void
 * Output: void
 * Prints info on how to launch the server
 */
static inline void
usage(void)
{
    printf("To launch server with default system parameters:\n");
    printf("\t./server\n");
    printf("To launch server with custom sytem parameters:\n");
    printf("\t./server -p <port> -f <file> -n <num of threads> -m <num of messages> -t <timeout>\n");
    printf("\t\tp: port to connect clients\n");
    printf("\t\tf: file is file to write to\n");
    printf("\t\tn: num of threads to process client requests\n");
    printf("\t\tm: num of continuous msgs from a client before yielding\n");
    printf("\t\tt: timeout before disconnecting a client if no data is received\n");
}

/*
 * Input: command line args
 * Output: 0 if there is no error, 1 if there is an error
 * Parses command line arguments and sets system params
 */
static uint8_t
parseArgs(int argc, char *argv[])
{
    extern char *optarg;
    extern int optind;
    int opt; 
    int ret = 1;  
    while((opt = getopt(argc, argv, ":p:f:n:t:m:h:")) != -1)  
    {  
        switch(opt)  
        {  
            case 'f':
                snprintf(sysParams.file_name, FILE_NAME_SZ, "%s", optarg);  
                break;
            case 'n':
                sysParams.num_of_threads = atoi(optarg);
                break; 
            case 't':  
                sysParams.client_timeout = atoi(optarg);
                break;  
            case 'p':
                sysParams.port = atoi(optarg);
                break;
            case 'h':
                usage();
                break;
            case ':':  
                printf("option needs a value\n");  
                usage();
                ret = 0;
                break;  
            case '?':  
                printf("unknown option: %c\n", optopt); 
                usage();
                ret = 0;
                break;  
        }  
    } 
      
    return ret;  
}

/*
 * Input: void
 * Output: void
 * Prints server params
 */
static inline void 
printSystemParams(void)
{
    printf("System parameters: file_name:%s, port:%u, threads:%u, timeout:%u num_msgs:%u\n",
            sysParams.file_name, sysParams.port, sysParams.num_of_threads, sysParams.client_timeout, sysParams.max_continuous_msgs);
}

/*
 * Input: clientfd
 * Output: void
 * Communicates with client
 * Disconnects client if there is no data within sysParams.client_timeout
 * Yields the client after processing max_conitnuous_msgs number of msgs
 */
static void
processClient (int clientfd)
{
        char recvBuff[MAX_MSG_SZ], sendBuff[MAX_MSG_SZ];
        int a, result;
        int numbytes = 0;

        Message msg;

        int numMsgs = 0;
        while(1)
        {
            memset(recvBuff, '0', sizeof(recvBuff));
            memset(sendBuff, '0', sizeof(sendBuff)); 

            /*receive data from the client*/
            numbytes = recv(clientfd,(void *)&msg, sizeof(msg),0);

            /*No data from client within timeout, connection to be closed*/
            if (numbytes == -1) {

                if (errno == EAGAIN)
                {
                    printf("No data from client within timeout: %d\n", sysParams.client_timeout);
                    close(clientfd);
                    break;
                }
            }
            if (numbytes == 0) {
                break;
            }

            /*Write msg to the file*/
            writeToFile(&msg);
#ifdef DBG
            printf("Message:received: id:%d, data:%s\n", msg.id, msg.data);
#endif          
            sprintf(sendBuff, "Message:%d written to server file",msg.id);

            /*Send response to client*/
            send(clientfd, sendBuff, strlen(sendBuff),0); 
           
            numMsgs++;

            /*Too many msgs from this client, lets put it back to clientQ*/
            if(numMsgs == sysParams.max_continuous_msgs)
            {
                addClient(clientfd);
                break;
            }

        }
}

/* 
 * Input: clientfd
 * Output: void
 * Enqueues client to clientQ and signals waiting worker threads
 * Main thread enqueues client connection to clientQ.
 * Worker thread enqueues client after processing max_continuous_msgs
 * qMutex to protect access to clientQ
 */
static void
addClient(int clientfd)
{

        pthread_mutex_lock(&qMutex);
        enqueue(clientQ, clientfd);
        pthread_mutex_unlock(&qMutex);

        /* Signal worker threads */
        pthread_cond_signal(&cond);
}

/*
 * Input: void
 * out put: clientfd
 * Dequeues client from clientQ
 * Waits on conditional variable for connections in clientQ
 * qMutex to protect access to clientQ
 */
static int
getClient(void)
{
        int clientfd;
        pthread_mutex_lock(&qMutex);

        /*If no clients in clientQ, wait on conditional variable cond till main threads signals*/
        while(isEmpty(clientQ))
        {
#ifdef DBG
                printf("Thread %lu: \tWaiting for Connection\n", pthread_self());
#endif
                if(pthread_cond_wait(&cond, &qMutex) != 0)
                {
                    perror("Cond Wait Error");
                }
        }

        clientfd = dequeue(clientQ);

        pthread_mutex_unlock(&qMutex);

        return clientfd;
}

/* Input: void
 * Output: void
 * Callback function to pthread.
 * Dequeues a client from clientQ and starts processing
 */
static void *
connectionHandler()
{
        int clientfd = 0;

        while(1)
        {
                clientfd = getClient();
#ifdef DBG
                printf("Thread %lu processing client: %d\n", pthread_self(), clientfd);
#endif      
                processClient(clientfd);
        }
}

/*
 * Main function
 */
int
main(int argc, char *argv[])
{
        int i = 0;
        int clientfd = -1;
        int ret=0;
        int listenfd = 0;
        struct sockaddr_in servAddr; 
        struct timeval tv;
        int enable = 1;
        int rv = 0;
        static client_cnt=0;

        /*Set system configuration to default values
         * if there is no command line configurtion
         */
        setDefaultSystemParams();

        /*parse command line system params*/
        if(!parseArgs(argc, argv))
            return 0;

        /*print final sytem params*/
        printSystemParams();

        /*Create and initialize clientQ that holds client fds*/
        Q q;
        clientQ = &q;
        initQ(clientQ);
       
        /*Open file to write msgs from clients*/
        fd = fopen(sysParams.file_name, "w+");

        if (fd == NULL)
        {
            printf("File open failed\n");
            return 0;
        }

        /*Initialize the mutex global variable
         * qMutex: To synchronize writing to clientQ
         * fMutex: To synchronize writing to file
         */
        pthread_mutex_init(&qMutex,NULL);
        pthread_mutex_init(&fMutex,NULL);

        /*create thread pool to process client conenctions*/
        pthread_t threadPool[sysParams.num_of_threads];

        /*Socket creation and binding*/
        listenfd = socket(AF_INET, SOCK_STREAM, 0);

        /*To avoid Socket Bind: Address already in use*/
        setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)); 

        if (listenfd <  0) 
        {
                perror("Error in socket creation");
                exit(1);
        }

        memset(&servAddr, '0', sizeof(servAddr));
        servAddr.sin_family = AF_INET;
        servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
        servAddr.sin_port = htons(sysParams.port); 

        rv = bind(listenfd, (struct sockaddr*)&servAddr, sizeof(servAddr)); 

        if (rv <  0) 
        {
                perror("Error in binding");
                exit(1);
        }

        /*Create Thread Pool*/
        for(i = 0; i < sysParams.num_of_threads; i++)
        {
                pthread_create(&threadPool[i], NULL, connectionHandler, (void *) NULL);
        }

        listen(listenfd, 10); 

        /* Accept the incoming connections from clients and enqueue them*/
        while(1)
        {
#ifdef DBG
                printf("Waiting for clients\n");
#endif

                clientfd = accept(listenfd, (struct sockaddr*)NULL, NULL);

                if(clientfd == -1)
                {
                    printf("Error in connecting client\n");
                }

                tv.tv_sec = sysParams.client_timeout;
                tv.tv_usec = 0;
                setsockopt(clientfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
#ifdef DBG
                printf("client:%d client:%d connected and enqueued\n", client_cnt, clientfd);
#endif                
                client_cnt++;

                /*Enqueue the client into clientQ*/
                addClient(clientfd); 

        }

        fclose(fd);
 
}
