#ifndef _MY_QUEUE_H
#define _MY_QUEUE_H
#include <stdbool.h>

/*Node to hold clientfd*/
typedef struct node {
    int client_fd;
    struct node *next;
} Node;

/*Queue to hold clients*/
typedef struct {

    Node *head;
    Node *tail;
    int size;
} Q;

/*Function headers*/
void initQ(Q *clientQ);
void enqueue(Q *clientQ, int client_fd);
int  dequeue(Q *clientQ);
bool isEmpty(Q *clientQ);

#endif
