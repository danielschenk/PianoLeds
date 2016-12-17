/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 16 Dec 2016
 * 
 * @brief CallbackList implementation.
 */

#include <assert.h>

#include "CallbackList.h"

/** Node definition. */
struct Node
{
    /** Pointer to next node in the list. */
    Node_t *next;
    
    /** Pointer to callback function. */
    Callback_t callback;   
};

static Node_t *New(Callback_t callback)
{
    Node_t *node = malloc(sizeof(Node_t));
    assert(node);
    node->callback = callback;
    node->next = NULL;
    
    return node;
}

void CallbackList_Initialize(CallbackList_t *list)
{
    list->first = NULL;
}

void CallbackList_Add(CallbackList_t *list, Callback_t callback)
{
    if(NULL == list->first)
    {
        /* List is empty. */
        list->first = New(callback);
    }
    else
    {
        Node_t *current = list->first;
        /* Find the last node (the only node which does not point to a next node). */
        while(NULL != current->next)
        {
            current = current->next;
        }
        current->next = New(callback);
    }
}

void CallbackList_Remove(CallbackList_t *list, Callback_t callback)
{
    if(NULL != list->first)
    {
        Node_t *current = list->first;
        Node_t *previous = NULL;
        do {
            if(current->callback == callback)
            {
                if(list->first == current)
                {
                    list->first = current->next;
                }
                else if(NULL != previous)
                {
                    previous->next = current->next;
                }
            }
            previous = current;
            current = current->next;
        } while(NULL != current);        
    }
}

void CallbackList_ProcessAll(CallbackList_t *list, void *arg)
{
    Node_t *current = list->first;
    while(NULL != current)
    {
        current->callback(arg);
        current = current->next;
    }
}
