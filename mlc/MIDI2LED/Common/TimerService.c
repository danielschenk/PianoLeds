/**
 * @file
 * @copyright (c) Daniel Schenk, 2016
 * This file is part of MLC: MIDI Led strip Controller.
 * 
 * @date 20 Dec 2016
 * 
 * @brief Timer service implementation.
 */

#include <assert.h>

#include "TimerService.h"

#define NUM_SLOTS 10

/** Timer definition. */
struct Timer
{
    /** Expiry time. */
    Tick_t expiresAt;
    
    /** Period for periodic timers. */
    Tick_t period;
    
    /** Pointer to callback function. */
    TimerCallback_t callback;
};

static int FindFreeSlot();

/** Pointer to function used for getting the actual tick count. */
static GetTickFunction_t gs_GetTickFunction = NULL;

/** List of timers. */
static struct Timer gs_Timers[NUM_SLOTS];

static TimerId_t FindFreeSlot()
{
    TimerId_t slot = TIMERID_INVALID;
    
    for(int i = 0; i < NUM_SLOTS; ++i)
    {
        if(NULL == gs_Timers[i].callback)
        {
            slot = (TimerId_t)i;
            break;
        }
    }
    
    return slot;
}

static void Schedule(TimerId_t timer, Tick_t expiresInMs, bool periodic)
{
    assert(NULL != gs_GetTickFunction);
    Tick_t now = gs_GetTickFunction();
    Tick_t period = MS_TO_TICKS(expiresInMs);
    
    gs_Timers[timer].expiresAt = now + period;
    gs_Timers[timer].period    = periodic ? period : 0;
}

void TimerService_Initialize(GetTickFunction_t getTickFunction)
{
    gs_GetTickFunction = getTickFunction;
    
    for(int i = 0; i < NUM_SLOTS; ++i)
    {
        /* Null callback denotes a free timer slot. Other values
         * are initialized upon creating a timer. */
        gs_Timers[i].callback = NULL;
    }
}

TimerId_t TimerService_Create(Tick_t expiresInMs, TimerCallback_t callback, bool periodic)
{
    TimerId_t newTimer = FindFreeSlot();
    
    if(TIMERID_INVALID != newTimer)
    {
        Schedule(newTimer, expiresInMs, periodic);
        gs_Timers[newTimer].callback  = callback;
    }
    
    return newTimer;
}

void TimerService_Reschedule(TimerId_t timer, Tick_t expiresInMs, bool periodic)
{
    if(timer >= 0 && timer < NUM_SLOTS)
    {
        if(NULL != gs_Timers[timer].callback)
        {
            Schedule(timer, expiresInMs, periodic);
        }
    }
}

void TimerService_Delete(TimerId_t timer)
{
    if(timer >= 0 && timer < NUM_SLOTS)
    {
        gs_Timers[timer].callback = NULL;
    }
}

void TimerService_Run()
{
    assert(NULL != gs_GetTickFunction);
    Tick_t now = gs_GetTickFunction();
    struct Timer *pTimer;
    
    for(int i = 0; i < NUM_SLOTS; ++i)
    {
        pTimer = &gs_Timers[i];
        if(NULL != pTimer->callback)
        {
            if((signed long)pTimer->expiresAt - (signed long)now < 0)
            {
                /* Inform the creator. */
                pTimer->callback((TimerId_t)i);
                
                if(pTimer->period > 0)
                {
                    /* Restart. */
                    pTimer->expiresAt = now + pTimer->period;
                }
                else
                {
                    /* Delete. */
                    pTimer->callback = NULL;
                }
            }
        }
    }
}

