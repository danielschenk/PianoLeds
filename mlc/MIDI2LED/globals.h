/**
* @file globals.h
* @brief Global definitions
* 
*
* @author Daniël Schenk
*
* @date 2011-12-07
*/


#ifndef GLOBALS_H_
#define GLOBALS_H_
#define F_CPU 20000000UL //!< CPU clock frequency

#define BUILD_DISPLAY 1

#define TICKS_TO_MS(ticks) (ticks*10)
#define MS_TO_TICKS(ms) (ms/10)

#endif /* GLOBALS_H_ */
