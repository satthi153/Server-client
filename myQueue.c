/*
 * Queue implementation
 * clientfds are stored in clientQ
 */

#include <stdio.h>
#include <stdlib.h>
#include "myQueue.h"
#include <assert.h>

/*
 * Input: ClientQ
 * Output: void
 * Initialize clientQ
 */
void
initQ(Q *clientQ)
{
   clientQ->head = clientQ->tail = NULL;
   clientQ->size = 0;
}

/*
 * Input: clientQ, clientfd
 * Output: void
 * Inserts clientfd at tail of the clientQ
 */
void
enqueue(Q *clientQ, int client_fd) {

    Node *new_node = (Node *)malloc(sizeof(Node *));
    if(!new_node)
    {
        assert(0);
        return;
    }
    new_node->next = NULL;
    new_node->client_fd = client_fd;

    /*clientQ is empty, make new node as head and tail of the queue*/
    if (clientQ->tail == NULL) 
    { 
        clientQ->head = clientQ->tail = new_node; 
        return; 
    } 

    clientQ->tail->next = new_node;
    clientQ->tail = new_node;

    clientQ->size++;

    return;
}

/*
 * Input: clientQ
 * Output: clienfd
 * Deletes clientfd from the head of the Q and returns it
 */
int
dequeue(Q *clientQ) {

    /*Is the Q empty?*/
    if(clientQ->head == NULL)
    {
        return -1;
    }

    int client_fd = clientQ->head->client_fd;
    Node *temp = clientQ->head;
    
    clientQ->head = temp->next;
    if(temp->next == NULL)
    {
        clientQ->tail = NULL;
    }

    
    free(temp);
    temp = NULL;

    clientQ->size--;

    return client_fd;
}

/*
 * Input: clientQ
 * Output: TRUE if Q is empty, else FALSE
 * Check if the Q is empty
 */
bool
isEmpty(Q *clientQ) {

    return (clientQ->head == 0);
}
