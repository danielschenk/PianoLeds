/**
* @file timer.c
* @brief Timer related definitions
* 
*
* @author Daniël Schenk
*
* @date 2011-12-07
*/


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

#define TICKS_TO_MS(ticks) (ticks*10)
#define MS_TO_TICKS(ms) (ms/10)

typedef uint32_t Tick_t;

void timerInit();


#endif /* TIMER_H_ */
