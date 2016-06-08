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

#include "ledstrip.h"
#include "BV4513.h"
#include "midi.h"
#include "timer.h"
#include "version.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>

#define BRIGHTNESS_IDLE 3
#define BRIGHTNESS_WARN 25

typedef uint32_t tick_t;

extern unsigned int midiIndicatorSet;
/* This is read from timer interrupt! */
static volatile bool g_enable_indicators = false;

static tick_t g_tick_count = 0;

void toggleHeartBeatLed()
{
	static unsigned char heartBeadLed = 0;
	heartBeadLed = !heartBeadLed;
	BV4513_setDecimalPoint(3, heartBeadLed);
}

static void displayFirmwareVersion()
{
	BV4513_clear();
	
	#if (VERSION_MINOR/10 >= 10)
	static const char fmt[] PROGMEM = "v%1u.%2u";
	#else
	static const char fmt[] PROGMEM = "v %1u.%1u";
	#endif
	
	char s[8];
	sprintf_P(s, fmt, VERSION_MAJOR, VERSION_MINOR);
	BV4513_writeString(s, 0);
}

static void displayBuildNumber()
{
	BV4513_clear();
	static const char fmt[] PROGMEM = "%s%3u";
	const char * prefix;
	#ifdef Debug
	prefix = "d";
	#else
	prefix = "b";
	#endif
	
	char s[5];
	sprintf_P(s, fmt, prefix, VERSION_COMMITS_PAST_TAG);
	BV4513_writeString(s, 0);
}

static void displayLedMode(unsigned int value)
{
	BV4513_clear();
	char s[5];
	static const char fmt[] PROGMEM = "P%3u";
	sprintf_P(s, fmt, value);
	BV4513_writeString(s, 0);
}

static void tickHook(tick_t curr_tick)
{
	/* Note: interrupts are already re-enabled by the tick interrupt
	 * before this hook gets called!*/
	
	/* Time at which the display brightness was bumped. 0 means display
	 * isn't currently bumped (is at idle brightness). */
	static tick_t brightness_bump = 0;
	/* Last led mode written to the display. */
	static unsigned int ledModePrevious = 0;
	
	if(ledMode != ledModePrevious)
	{
		displayLedMode(ledMode);
		/* Bump brightness */
		BV4513_setBrightness(BRIGHTNESS_WARN);
		brightness_bump = curr_tick;
		if(brightness_bump == 0) /* 0 means no brightness reset needed so prevent this value */
			brightness_bump++;
		ledModePrevious = ledMode;
	}
	
	if(brightness_bump > 0 && curr_tick - brightness_bump >= MS_TO_TICKS(1000))
	{
		BV4513_setBrightness(BRIGHTNESS_IDLE);
		brightness_bump = 0;
	}
}

int main(void)
{
	//----------------------COMMON INITIALIZATIONS FOR ALL BUILDS--------------------------------
	wdt_enable(WDTO_1S);
	sei();
	
	CLKPR = 0x00; //No CPU clock prescaling
	MCUCR = (0<<JTD);
	
	/* Read reset cause flags. */
	uint8_t mcusr = MCUSR;
	/* Clear all flags, so next MCU reset won't have an ambiguous cause. */
	MCUSR = 0;
	
	if(mcusr & (1<<PORF) || mcusr & (1<<BORF)) {
		/* Power-on reset or brown-out reset. */
		#if BUILD_DISPLAY
		/* Wait a while so display can power-up properly. */
		_delay_ms(200);
		wdt_reset();
		#endif
	}
	
	//----------------------VARIOUS TESTING BUILDS-----------------------------------------------
	#if BUILD_DISPLAYTEST
	BV4513_init();
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
		
		for(char c = '0'; c <= 'z'; c++)
		{
			char s[] = {c, 0};
			BV4513_writeString(s, 3);
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
	
	#if BUILD_DISPLAY
	BV4513_init();
	wdt_reset();


	displayFirmwareVersion();
	_delay_ms(500);
	wdt_reset();
	displayBuildNumber();
	_delay_ms(500);
	wdt_reset();
	BV4513_clear();
	displayLedMode(ledMode);
	//g_enable_indicators = false;
	#endif
	
	ledSetAutoWrite(0);
	
	//uint8_t ledTestSetpoint = 255;
	
	/* Enables the tick interrupt which triggers periodic events */
	timerInit();
	while(1) //Keep waiting for interrupts
    {
		wdt_reset();
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

/* Tick interrupt */
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
	
	g_tick_count++;
	sei();
	tickHook(g_tick_count);
}

