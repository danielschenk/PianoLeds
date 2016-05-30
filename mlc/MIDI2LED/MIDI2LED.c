/**
* @file MIDI2LED.c
* @brief Main MIDI2LED code
* 
*
* @author Daniël Schenk
*
* @date 2011-09-28
*/
#include "globals.h"
//#define F_CPU 20000000

//#define led_indication

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>


#include "ledstrip.h"
#include "BV4513.h"
#include "midi.h"
#include "timer.h"
#include "version.h"

//#include "TWI_Master.h"


// extern uint8_t ledsR[ledsConnected];
// extern uint8_t ledsG[ledsConnected];
// extern uint8_t ledsB[ledsConnected];
// 
extern unsigned int midiIndicatorSet;
/* This is read from timer interrupt! */
static volatile bool g_enable_indicators = false;

unsigned char dummy = 0;

void toggleHeartBeatLed();

void toggleHeartBeatLed()
{
	static unsigned char heartBeadLed = 0;
	heartBeadLed = !heartBeadLed;
	BV4513_setDecimalPoint(3, heartBeadLed);
}

static void displayFirmwareVersion()
{
	BV4513_clear();
	const char * fmt;
	int pos;
	if(VERSION_MINOR/10 >= 10) {
		fmt = "v%1u.%2u";
		pos = 0;
	}
	else {
		fmt = "v%1u.%1u";
		pos = 1;
	}
	
	char s[8];
	sprintf(s, fmt, VERSION_MAJOR, VERSION_MINOR);
	BV4513_writeString(s, pos);
}

int main(void)
{
	//----------------------COMMON INITIALIZATIONS FOR ALL BUILDS--------------------------------
	CLKPR = 0x00; //No CPU clock prescaling
	MCUCR = (0<<JTD);
	
	//----------------------VARIOUS TESTING BUILDS-----------------------------------------------
	#if BUILD_DISPLAYTEST
	BV4513_init();
	sei();
	while (1)
	{
		for(char c = '0'; c <= '9'; c++)
		{
			char s[] = {c, 0};
			BV4513_writeString(s, 0);
			BV4513_writeDigit(c-48, 1);
			while(TWI_Transceiver_Busy());
			_delay_ms(500);
		}
		
		//BV4513_writeDigit(1, 1);
		//_delay_ms(500);
		//BV4513_writeDigit(2, 1);
		//_delay_ms(500);
		//
		//for(int n=9; n<10000; n++)
		//{
			//BV4513_writeNumber(n);
			//_delay_ms(50);
		//}
	}
	
	#elif BUILD_MIDITODISPLAYTEST
	BV4513_init();
	midiInit();
	sei();
	while (1)
	{
		
	}
	
	#elif BUILD_BASICTEST
	DDRD = 0xFF;
	while(1)
	{
		PORTD = 0x00;
		_delay_ms(1);
		PORTD = 0xFF;
		_delay_ms(1);
	}
	
	#elif BUILD_LEDTEST
	ledInit();
	sei(); //Enable global interrupts
	ledSetAutoWrite(1);
	
	ledMode = 0;
	
	while(1) //Keep waiting for interrupts
    {
		ledTestLoops();				
    }
	
	#else
	//---------------------DEFAULT OR DEBUG BUILD-------------------------------
	ledInit();
	midiInit();
	timerInit();
	
	#if BUILD_DISPLAY
	BV4513_init();
	#endif
	sei(); //Enable global interrupts
	
	#if BUILD_DISPLAY
	displayFirmwareVersion();
	_delay_ms(2000);
	BV4513_clear();
	g_enable_indicators = true;
	//BV4513_writeNumber(ledMode);
	#endif
	
	ledSetAutoWrite(0);
	
	//uint8_t ledTestSetpoint = 255;
	
	while(1) //Keep waiting for interrupts
    {
		if(ledMode==0)
		{
			ledTestLoops();
			//ledSingleColorSetLed(5,5,5,0);
		}
		asm("NOP"); //"No operation" to overcome strange behavior (program pointer stuck at previous statement)
		
		#ifdef led_indication
		ledSingleColorSetLed(5,5,5,0);
		_delay_ms(100);
		ledSingleColorSetLed(0,0,0,0);
		_delay_ms(100);
		asm("NOP");
		#endif
    }
	
	#endif
}

ISR(USART0_RX_vect)
{
	#ifdef Debug
	ledSingleColorSetLed(255,255,255,1);
	#endif
	
	midiHandleByte();
	//dummy = 0;
	
	#if BUILD_MIDITODISPLAYTEST
	midiDisplayNote();
	#endif
	
	#ifdef Debug
	ledSingleColorSetLed(0,0,0,1);
	#endif
}

ISR(USART1_TX_vect)
{
// 	#ifdef Debug
// 	ledSingleColorSetLed(0,15,0,2);
// 	#endif
	
	ledWriteNextByte();
	
// 	#ifdef Debug
// 	ledSingleColorSetLed(0,0,0,2);
// 	#endif
}

ISR(TIMER0_COMPA_vect)
{
	ledEndPause();
}

ISR(TIMER1_COMPA_vect)
{
	static uint8_t renderFreqDiv = 0;
	static uint8_t heartBeatLedCount = 0;
	static uint8_t midiIndicatorCount = 0;
	#if BUILD_DISPLAY
	if(g_enable_indicators && midiIndicatorSet)
	{
		if(midiIndicatorCount>=100)
		{
			midiIndicatorCount = 0;
			midiIndicator(0);
		}
		else
			midiIndicatorCount++;
	}
	
	heartBeatLedCount++;
	
	if(g_enable_indicators && heartBeatLedCount>=50)
	{
		toggleHeartBeatLed();
		heartBeatLedCount = 0;
	}
	#endif
	if(renderFreqDiv == 3)
	{
		ledRenderAfterEffects(ledMode);
	}
	
	if(renderFreqDiv >= 3)
	{
		renderFreqDiv = 0;
	}
	else
	{
		renderFreqDiv++;
	}
	
	if (ledAutoWrite==0)
	{
		ledWriteNextByte();
	}
}

