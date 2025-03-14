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
#include "Model/ConfigurationModel.h"
#include "Common/TimerService.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdbool.h>

#define BRIGHTNESS_IDLE 3
#define BRIGHTNESS_WARN 25
#define DISPLAY_DIM_TIMEOUT_MS 5000
#define LAST_PRESET_XOR_MASK ((uint8_t)0x5C)

extern unsigned int midiIndicatorSet;
/* This is read from timer interrupt! */
static volatile bool g_enable_indicators = false;

/** Timer ID of dim timer, @ref TIMERID_INVALID if not running. */
static TimerId_t gs_dimTimer = TIMERID_INVALID;

static Tick_t g_tick_count = 0;

/** Last preset, preserved across reboots (provided that power supply was stable enough to preserve SRAM),
 * an XOR'ed value is kept too to be able to do an extra check.
 */
static uint8_t g_lastPreset __attribute__((section(".noinit")));
static uint8_t g_lastPresetCheck __attribute__((section(".noinit")));

static Tick_t GetTickCount()
{
    return g_tick_count;
}

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
	/* LED mode is the raw MIDI program number (0-based). Most instruments display it as 1-based.
	 * Increment with 1 to match that */
	sprintf_P(s, fmt, value + 1);
	BV4513_writeString(s, 0);
}

static void DisplayDimTimerCallback(TimerId_t unused)
{
    BV4513_setBrightness(BRIGHTNESS_IDLE);
    gs_dimTimer = TIMERID_INVALID;
}

static void DisplayPresetChangedCallback(void *arg)
{
    uint8_t newPreset = *(uint8_t *)arg;
	g_lastPreset = newPreset;
	g_lastPresetCheck = newPreset ^ LAST_PRESET_XOR_MASK;

#warning "TODO: Remove this dirty workaround"
    /* The display driver needs interrupts. */
    sei();

    displayLedMode(newPreset);

    /* Bump brightness */
    BV4513_setBrightness(BRIGHTNESS_WARN);

    if(TIMERID_INVALID == gs_dimTimer)
    {
        gs_dimTimer = TimerService_Create(DISPLAY_DIM_TIMEOUT_MS, DisplayDimTimerCallback, false);
    }
    else
    {
        TimerService_Reschedule(gs_dimTimer, DISPLAY_DIM_TIMEOUT_MS, false);
    }
}

int main(void)
{
	//----------------------COMMON INITIALIZATIONS FOR ALL BUILDS--------------------------------
	wdt_enable(WDTO_2S);
	sei();

	CLKPR = 0x00; //No CPU clock prescaling
	MCUCR = (0<<JTD);

	/* Read reset cause flags. */
	uint8_t mcusr = MCUSR;
	/* Clear all flags, so next MCU reset won't have an ambiguous cause. */
	MCUSR = 0;

	bool powerOnReset = mcusr & (1<<PORF);
	bool brownOutReset = mcusr & (1<<BORF);
	bool watchdogReset = mcusr & (1<<WDRF);
	bool externalReset = mcusr & (1<<EXTRF);

	if(powerOnReset|| brownOutReset) {
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

	#else
	//---------------------DEFAULT OR DEBUG BUILD-------------------------------
    ConfigurationModel_Initialize();
    TimerService_Initialize(GetTickCount);

	ledInit();
	midiInit();

	#if BUILD_DISPLAY
	BV4513_init();
	wdt_reset();

	if (brownOutReset && !powerOnReset)
	{
		BV4513_writeString("Ebor", 0);
		_delay_ms(500);
		wdt_reset();
	}
	if (watchdogReset)
	{
		BV4513_writeString("Ewdt", 0);
		_delay_ms(500);
		wdt_reset();
	}
	if (externalReset)
	{
		BV4513_writeString("E Er", 0);
		_delay_ms(500);
		wdt_reset();
	}

	displayFirmwareVersion();
	_delay_ms(500);
	wdt_reset();
	if (VERSION_COMMITS_PAST_TAG > 0)
	{
		displayBuildNumber();
		_delay_ms(500);
	}
	wdt_reset();
	BV4513_clear();

	ConfigurationModel_SubscribeCurrentPreset(DisplayPresetChangedCallback);
	if (!powerOnReset && !brownOutReset)
	{
		if (g_lastPreset ^ LAST_PRESET_XOR_MASK == g_lastPresetCheck)
		{
			ConfigurationModel_SetCurrentPreset(g_lastPreset);
		}
	}
	displayLedMode(ConfigurationModel_GetCurrentPreset());
	//g_enable_indicators = false;
	#endif

	//uint8_t ledTestSetpoint = 255;

	/* Enables the tick interrupt which triggers periodic events */
	timerInit();

    /* Initial dim */
    gs_dimTimer = TimerService_Create(5000, DisplayDimTimerCallback, false);

    while(1) //Keep waiting for interrupts
    {
		wdt_reset();

        /* Service the timers */
        TimerService_Run();

		//if(ConfigurationModel_GetCurrentPreset() == 0)
		//{
			//ledTestLoops();
			////ledSingleColorSetLed(5,5,5,0);
		//}
		asm("NOP"); //"No operation" to overcome strange behavior (program pointer stuck at previous statement)
    }

	#endif
}

ISR(USART0_RX_vect)
{
	#ifdef Debug
	ledSingleColorSetLed(255,255,255,1);
	#endif

#warning "TODO: redesign this, only fill buffer from interrupt and do processing from main"
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
		ledRenderAfterEffects(ConfigurationModel_GetCurrentPreset());
	}

	if(renderFreqDiv >= 3)
	{
		renderFreqDiv = 0;
	}
	else
	{
		renderFreqDiv++;
	}

	ledWriteNextByte();

	g_tick_count++;
	sei();
}
