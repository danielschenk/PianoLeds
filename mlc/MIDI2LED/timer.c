/**
* @file timer.c
* @brief Timer related functions
* 
*
* @author Daniël Schenk
*
* @date 2011-12-07
*/

#include "timer.h"
#include "ledstrip.h"
#include <avr/io.h>

void timerInit()
{
	TCNT1 = 0;
	TCCR1B = (0<<WGM13|1<<WGM12|0<<CS12|1<<CS11|0<<CS10); //Start timer, CLK/8, CTC mode
	TIMSK1 = (1<<OCIE1A); //Enable output compare match 1 interrupt
	OCR1A = 0x61A7; //100 Hz with prescale 8
}

