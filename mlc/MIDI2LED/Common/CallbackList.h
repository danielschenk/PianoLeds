/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 16 Dec 2016
 * 
 * @brief CallbackList interface.
 */


#ifndef CALLBACKLIST_H_
#define CALLBACKLIST_H_

/** Node type. */
typedef struct Node Node_t;

/** List definition. */
struct CallbackList
{
    /** Pointer to first node in the list. */
    Node_t *first;
};

/** Callback list type. */
typedef struct CallbackList CallbackList_t;

/** Callback function pointer type. */
typedef void(*Callback_t)(void*);

/**
 * Initialize the callback list.
 * 
 * @param list  The list.
 */
void CallbackList_Initialize(CallbackList_t *list);

/**
 * Add a callback to the list.
 * 
 * @param list      The list.
 * @param callback  The callback to add.
 */
void CallbackList_Add(CallbackList_t *list, Callback_t callback);

/**
 * Remove a callback from the list.
 * 
 * @param list      The list.
 * @param callback  The callback to remove.
 */
void CallbackList_Remove(CallbackList_t *list, Callback_t callback);

/**
 * Call every callback on the list with the given argument.
 * 
 * @param list      The list.
 * @param arg       The argument to pass to every callback.
 */
void CallbackList_ProcessAll(CallbackList_t *list, void *arg);


#endif /* CALLBACKLIST_H_ */
