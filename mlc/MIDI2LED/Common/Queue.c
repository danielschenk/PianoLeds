/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 26 Dec 2016
 * 
 * @brief Queue implementation.
 */

#include <string.h>

#include "Queue.h"

static bool IsFull(const Queue_t *pQueue);
static bool IsEmpty(const Queue_t *pQueue);
static const void *EndOfStorage(const Queue_t *pQueue);

void Queue_Initialize(Queue_t *pQueue, void *pQueueStorage, size_t itemSize, unsigned int numberOfItems)
{   
    pQueue->pStorage         = pQueueStorage;
    pQueue->pHead            = pQueueStorage;
    pQueue->maxNumberOfItems = numberOfItems;
    pQueue->itemSize         = itemSize;
    pQueue->count            = 0;
}

bool Queue_Pop(Queue_t *pQueue, void *pTo)
{
    bool success = false;
    
    if(!IsEmpty(pQueue))
    {
        memcpy(pTo, pQueue->pHead, pQueue->itemSize);
        --pQueue->count;
        pQueue->pHead += pQueue->itemSize;
        /* Check for wrap-around. */
        if(pQueue->pHead >= EndOfStorage(pQueue))
        {
            pQueue->pHead = pQueue->pStorage;
        }
        
        success = true;
    }
    
    return success;
}

bool Queue_Push(Queue_t *pQueue, const void *pFrom)
{
    bool success = false;
    
    if(!IsFull(pQueue))
    {
        void *pNextFreeItem = pQueue->pHead + (pQueue->count * pQueue->itemSize);
        /* Check for wrap-around. */
        if(pNextFreeItem >= EndOfStorage(pQueue))
        {
            pNextFreeItem = pQueue->pStorage;
        }
        
        memcpy(pNextFreeItem, pFrom, pQueue->itemSize);
        ++pQueue->count;
        
        success = true;
    }
    
    return success;
}

static bool IsFull(const Queue_t *pQueue)
{
    return pQueue->count >= pQueue->maxNumberOfItems;
}

static bool IsEmpty(const Queue_t *pQueue)
{
    return 0 == pQueue->count;
}

static const void *EndOfStorage(const Queue_t *pQueue)
{
    return pQueue->pStorage + (pQueue->itemSize * pQueue->maxNumberOfItems);
}
