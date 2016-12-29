/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 26 Dec 2016
 * 
 * @brief Queue interface.
 */


#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Queue definition. */
struct Queue
{
    /** Pointer to data storage array. */
    void *pStorage;
    
    /** Head index (index of first item). */
    void *pHead;
    
    /** Number of items currently in the queue. */
    unsigned int count;
    
    /** Size of the data array. */
    size_t maxNumberOfItems;
    
    /** Size of a single item. */
    size_t itemSize;
};

/** Queue type. */
typedef struct Queue Queue_t;

/**
 * Initialize a queue structure.
 * 
 * @param pQueue        Pointer to queue structure.
 * @param pQueueStorage Pointer to queue storage area (must be at least @p itemSize * @p numberOfItems bytes).
 * @param itemSize      Size of a single item.
 * @param numberOfItems Maximum number of items in the queue.
 */
void Queue_Initialize(Queue_t *pQueue, void *pQueueStorage, size_t itemSize, unsigned int numberOfItems);

bool Queue_Pop(Queue_t *pQueue, void *pTo);

bool Queue_Push(Queue_t *pQueue, const void *pFrom);

#ifdef __cplusplus
}
#endif

#endif /* QUEUE_H_ */
