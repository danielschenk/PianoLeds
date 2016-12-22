/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 20 Dec 2016
 * 
 * @brief TimerService interface.
 */


#ifndef TIMERSERVICE_H_
#define TIMERSERVICE_H_

#include <stdbool.h>

#include "../timer.h"

#define TIMERID_INVALID ((TimerId_t)-1)

/** Timer ID type. */
typedef int TimerId_t;

/** Function pointer type for timer callback functions. */
typedef void(*TimerCallback_t)(TimerId_t);

/** Function pointer type for GetTickCount functions. */
typedef Tick_t(*GetTickFunction_t)();

/**
 * Initialize the timer service.
 * 
 * @param getTickFunction   Pointer to function which returns the actual tick count.
 */
void TimerService_Initialize(GetTickFunction_t getTickFunction);

/**
 * Create a timer.
 * 
 * @param expiresInMs   Time in ms after which the timer should expire.
 * @param callback      Pointer to callback function which should be called upon expiry.
 * @param periodic      Whether the timer should be restarted with the same period after expiry.
 * 
 * @retval   TIMERID_INVALID    Creating the timer failed.
 * @retval !=TIMERID_INVALID    Timer ID of the created timer.
 */
TimerId_t TimerService_Create(Tick_t expiresInMs, TimerCallback_t callback, bool periodic);

/**
 * Delete (stop) an existing timer.
 * 
 * @param timer Timer ID of the timer to be deleted.
 */
void TimerService_Delete(TimerId_t timer);

/**
 * Run the timer service.
 */
void TimerService_Run();


#endif /* TIMERSERVICE_H_ */
