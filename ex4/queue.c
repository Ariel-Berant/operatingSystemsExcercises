// TODO: 
//  1) Finish implementing the queue data structure
//  2) Create a "queue" for the threads(due to FIFO requirement), to take care of every operation.
//      Need to cycle threads(remove every dequeue if max number of threads is defined?). Also make sure to update passed every dequeue, and ensure that the queue is thread-safe. 


#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

typedef struct Node {
    void *data;
    struct Node* next;
} Node;

typedef struct queue {
    Node* head;
    Node* last;
    int passed;
} queue;

static queue* q;

Node* createNode(void* data) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    return node;
}

// Function to free a node
void freeNode(Node* node) {
    free(node->data);
    free(node);
}

// Function to create a node
Node* createNode(void* data) {
    Node* node = (Node*)malloc(sizeof(Node));
    node->data = data;
    node->next = NULL;
    return node;
}

// Function to initialize the queue data structure
void initQueue() {
}

// Function to clear the queue data structure
void destroyQueue() {
}

// Function to add an element to the queue
void enqueue(void* data) {
}

// Function to remove first element from the queue
void* dequeue() {
    return NULL;
}

// Function to try to remove the first element from the queue
void* tryDequeue() {
    return NULL;
}

// Function to get the number of elements passed in the queue
size_t visited() {
    return q->passed;
}
