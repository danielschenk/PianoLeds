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


#include "ledstrip.h"
#include "BV4513.h"
#include "midi.h"
#include "timer.h"

//#include "TWI_Master.h"


// extern uint8_t ledsR[ledsConnected];
// extern uint8_t ledsG[ledsConnected];
// extern uint8_t ledsB[ledsConnected];
// 
extern unsigned int midiIndicatorSet;


unsigned char dummy = 0;

void toggleHeartBeatLed();

void toggleHeartBeatLed()
{
	static unsigned char heartBeadLed = 0;
	heartBeadLed = !heartBeadLed;
	BV4513_setDecimalPoint(3, heartBeadLed);
}

int main(void)
{
	//----------------------COMMON INITIALIZATIONS FOR ALL BUILDS--------------------------------
	CLKPR = 0x00; //No CPU clock prescaling
	MCUCR = (0<<JTD);
	
	//----------------------VARIOUS TESTING BUILDS-----------------------------------------------
	#ifdef build_displaytest
	BV4513_init();
	sei();
	while (1)
	{
		BV4513_writeDigit(1, 1);
		_delay_ms(50);
		BV4513_writeDigit(2, 1);
		_delay_ms(50);
	}
	
	#elif build_miditodisplaytest
	BV4513_init();
	midiInit();
	sei();
	while (1)
	{
		
	}
	
	#elif build_basictest
	DDRD = 0xFF;
	while(1)
	{
		PORTD = 0x00;
		_delay_ms(1);
		PORTD = 0xFF;
		_delay_ms(1);
	}
	
	#elif build_ledtest
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
	//BV4513_init();
	ledInit();
	midiInit();
	timerInit();
	
	#ifdef displayOn
	BV4513_init();
	#endif
	
	//BV4513_writeNumber(ledMode);
	
	sei(); //Enable global interrupts
	
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
	
	#ifdef build_miditodisplaytest
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
	#ifdef displayOn
	if(midiIndicatorSet)
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
	
	if(heartBeatLedCount>=50)
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

