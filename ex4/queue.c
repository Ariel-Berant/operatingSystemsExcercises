// TODO: 
//  1) Finish implementing the queue data structure
//  2) Create a "queue" for the threads(due to FIFO requirement), to take care of every operation.
//      Need to cycle threads(remove every dequeue if max number of threads is defined?). Also make sure to update passed every dequeue, and ensure that the queue is thread-safe. 


#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <stdbool.h>

typedef struct Node {
    void *data;
    struct Node* next;
} Node;

typedef struct threadNode {
    cnd_t cond;
    struct threadNode* next;
} threadNode;

typedef struct threadQueue {
    threadNode* head;
    threadNode* last;
    size_t size;
} threadQueue;

typedef struct queue {
    Node* head;
    Node* last;
    int passed, actSize;
    threadQueue* threads;
    mtx_t lock;
} queue;

static queue* q;

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

// Function to get the ith element in the queue(for tryDequeue, not going to sleep), ONLY USE IN LOCKED STATE!!!
void* getIthElement(int i) {
    // Initialize the current head and previous node
    Node *currHead = q->head, *prev = NULL;
    // Iterate through the queue to get to the ith element
    for (int j = 0; j < i; j++)
    {
        prev = currHead;
        currHead = currHead->next;
    }
    // Get the data from the ith element, and free the node
    void* data = currHead->data;
    if (prev != NULL)
    {
        prev->next = currHead->next;
    }
    else
    {
        q->head = currHead->next;
    }
    freeNode(currHead);
    // Update the passed and actSize, and if the queue is empty, set the head and last to NULL(can't recongnize freed memory otherwise)
    q->passed++;
    q->actSize--;
    if (q->actSize == 0)
    {
        q->last = NULL;
        q->head = NULL;
    }
    return data;
}


// Function to initialize the queue data structure
void initQueue() {
    // Initialize the queue, its threadQueue and its lock
    q = (queue*)malloc(sizeof(queue));
    q->head = NULL;
    q->last = NULL;
    q->passed = 0;
    q->actSize = 0;
    q->threads = (threadQueue*)malloc(sizeof(threadQueue));
    q->threads->head = NULL;
    q->threads->last = NULL;
    q->threads->size = 0;
    mtx_init(&(q->lock), mtx_plain);

}

// Function to clear the queue data structure, can assume that the queue is empty, and no threads are waiting
void destroyQueue() {
    // Free the queue, its threadQueue and its lock
    mtx_destroy(&(q->lock));
    free(q->threads);
    free(q);
}

// Function to add an element to the queue
void enqueue(void* data) {
    mtx_lock(&(q->lock));
    // Create a new node and add it to the queue. If the queue is empty, set the head and last to the new node
    Node* node = createNode(data);
    if (q->head == NULL)
    {
        q->head = node;
        q->last = node;
        q->actSize++;
    }
    else
    {
        q->last->next = node;
        q->last = node;
        q->actSize++;
    }
    // If there are threads waiting, signal the first one to wake up, and remove it from the queueSS
    if (q->threads->size > 0)
    {
        // Signal the first thread to wake up, destroy the condition variable, free the threadNode and adjust the size
        cnd_signal(&q->threads->head->cond);
        cnd_destroy(&q->threads->head->cond);
        threadNode* temp = q->threads->head;
        q->threads->head = q->threads->head->next;
        free(temp);
        q->threads->size--;
    }
    mtx_unlock(&(q->lock));
}

// Function to remove first element from the queue
void* dequeue() {
    mtx_lock(&(q->lock));
    // If the queue is empty, wait for the next element to be added
    while (q->head == NULL)
    {
        // Create a new threadNode and add it to the queue
        threadNode* node = (threadNode*)malloc(sizeof(threadNode));
        node->next = NULL;
        cnd_init(&node->cond);
        // If the queue is empty, add the threadNode to the head, else add to the last
        if (q->threads->head == NULL)
        {
            q->threads->head = node;
        }
        else
        {
            q->threads->last->next = node;
        }
        q->threads->last = node;
        q->threads->size++;
        // Wait for the next element to be added
        cnd_wait(&q->threads->head->cond, &(q->lock));
    }
    // Get the data from the head of the queue, and free the appropriate node
    void* data = q->head->data;
    Node* temp = q->head;
    q->head = q->head->next;
    freeNode(temp);
    // Update the passed and actSize, and if the queue is empty, set the head and last to NULL(can't recongnize freed memory otherwise)
    q->passed++;
    q->actSize--;
    if (q->actSize == 0)
    {
        q->last = NULL;
        q->head = NULL;
    }
    mtx_unlock(&(q->lock));
    return data;
}

// Function to try to remove the first element from the queue
bool tryDequeue(void** data) {
    bool success = false;
    mtx_lock(&(q->lock));
    // If there is at least one element in the queue that the existing dequeues leave
    if (q->actSize - q->threads->size > 0)
    {
        // Get the logically 1st element(index of threads->size) in the queue
        *data = getIthElement(q->threads->size);
        success = true;
    }
    mtx_unlock(&(q->lock));
    return success;
}

// Function to get the number of elements passed in the queue
size_t visited() {
    return q->passed;
}
