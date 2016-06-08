/**
* @file ledstrip.c
* @brief LED strip related functions
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
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

uint8_t ledsR[ledsProgrammed]; //!<Red intensity values
uint8_t ledsG[ledsProgrammed]; //!<Green intensity values
uint8_t ledsB[ledsProgrammed]; //!<Blue intensity values

uint8_t ledMapping[88]; //!<Note number to LED number mapping. mapping[noteNr]==ledNr
//uint8_t currentLedColor=0;

enum ledWriteStateEnum ledWriteState = writeR;

uint8_t ledAutoWrite = 0; //!< Determines if a new strip write has to start automatically after a previous write has been completed
unsigned int ledMode; //!< LED effect mode

unsigned char rMax; //!< Red intensity maximum (varies according to effect mode)
unsigned char gMax; //!< Greem intensity maximum (varies according to effect mode)
unsigned char bMax; //!< Blue intensity maximum (varies according to effect mode)

unsigned char ledTestSetpoint = setpoint_high;

/**
* This method writes the mapping of note numbers to LED numbers in memory.
* @author Daniël Schenk
* @date 2011-09-29
*/
void ledCreateMapping() 
{
	#if ledMappingVersion == 1
	ledMapping[0]=0;
	ledMapping[1]=101;
	ledMapping[2]=1;
	ledMapping[3]=2;
	ledMapping[4]=99;
	ledMapping[5]=3;
	ledMapping[6]=98;
	ledMapping[7]=4;
	ledMapping[8]=5;
	ledMapping[9]=96;
	ledMapping[10]=6;
	ledMapping[11]=95;
	ledMapping[12]=7;
	ledMapping[13]=94;
	ledMapping[14]=8;
	ledMapping[15]=9;
	ledMapping[16]=92;
	ledMapping[17]=10;
	ledMapping[18]=91;
	ledMapping[19]=11;
	ledMapping[20]=12;
	ledMapping[21]=89;
	ledMapping[22]=13;
	ledMapping[23]=88;
	ledMapping[24]=14;
	ledMapping[25]=87;
	ledMapping[26]=15;
	ledMapping[27]=16;
	ledMapping[28]=85;
	ledMapping[29]=17;
	ledMapping[30]=84;
	ledMapping[31]=18;
	ledMapping[32]=19;
	ledMapping[33]=82;
	ledMapping[34]=20;
	ledMapping[35]=81;
	ledMapping[36]=21;
	ledMapping[37]=80;
	ledMapping[38]=22;
	ledMapping[39]=23;
	ledMapping[40]=78;
	ledMapping[41]=24;
	ledMapping[42]=77;
	ledMapping[43]=25;
	ledMapping[44]=26;
	ledMapping[45]=75;
	ledMapping[46]=27;
	ledMapping[47]=74;
	ledMapping[48]=28;
	ledMapping[49]=73;
	ledMapping[50]=29;
	ledMapping[51]=30;
	ledMapping[52]=71;
	ledMapping[53]=31;
	ledMapping[54]=70;
	ledMapping[55]=32;
	ledMapping[56]=33;
	ledMapping[57]=68;
	ledMapping[58]=34;
	ledMapping[59]=67;
	ledMapping[60]=35;
	ledMapping[61]=66;
	ledMapping[62]=36;
	ledMapping[63]=37;
	ledMapping[64]=64;
	ledMapping[65]=38;
	ledMapping[66]=63;
	ledMapping[67]=39;
	ledMapping[68]=40;
	ledMapping[69]=61;
	ledMapping[70]=41;
	ledMapping[71]=60;
	ledMapping[72]=42;
	ledMapping[73]=59;
	ledMapping[74]=43;
	ledMapping[75]=44;
	ledMapping[76]=57;
	ledMapping[77]=45;
	ledMapping[78]=56;
	ledMapping[79]=46;
	ledMapping[80]=47;
	ledMapping[81]=54;
	ledMapping[82]=48;
	ledMapping[83]=53;
	ledMapping[84]=49;
	ledMapping[85]=52;
	ledMapping[86]=50;
	ledMapping[87]=51;
	#endif
	
	#if ledMappingVersion == 2
	int bottom = 0;
	int top = 87;
	int noteNr = 0;
	while(noteNr<88)
	{
		ledMapping[noteNr] = bottom;
		bottom++;
		noteNr++;
		ledMapping[noteNr] = top;
		top--;
		noteNr++;
	}
	#endif
}

/**
* This method configures USART1 in SPI mode for LED strip communication. Based on example from ATmega164P data sheet.
* @author Daniël Schenk
* @param baud Desired baud rate
* @date 2011-09-29
*/
void ledInitUSART1SPI(long baud) 
{
	UBRR1 = 0;
	/* Setting the XCKn port pin as output, enables master mode. */
	DDRD |= 0b00010000;
	
	UCSR1C = (1<<UMSEL11)|(1<<UMSEL10)|(0<<2)|(0<<UCPHA1)|(0<<UCPOL1);
	
	UCSR1B = (0<<RXEN1)|(1<<TXEN1);
	/* Set baud rate. */
	/* IMPORTANT: The Baud Rate must be set after the transmitter is enabled
	*/
	UBRR1 = (F_CPU / (2*baud)) - 1;
	UCSR1B |= 0b01000000; /* Set TXCIE1 bit (enable transmit complete interrupts) */
}


/**
* This function is a call to all required initialization steps.
* @author Daniël Schenk
* @date 2011-09-29
*/
void ledInit()
{
	ledCreateMapping();
	ledInitUSART1SPI(ledBaud);
	ledSetAutoWrite(0);
	ledWriteNextByte();
	ledModeChange(ledInitMode);
}

/**
* This function writes intensity values for all LEDs into memory, according to current note velocities.
* @param r Red intensity
* @param g Green intensity
* @param b Blue intensity
* @author Daniël Schenk
* @date 2011-09-28
*/
void ledSingleColorUpdateFull(uint8_t r, uint8_t g, uint8_t b)
{
	for (int noteNr=0; noteNr<88; noteNr++)
	{
		ledsR[ledMapping[noteNr]]=notes[noteNr]*(r/255);
		ledsG[ledMapping[noteNr]]=notes[noteNr]*(g/255);
		ledsB[ledMapping[noteNr]]=notes[noteNr]*(b/255);
	}

}

/**
* This function writes a single LED intensity into memory, according to the current note velocity of the provided note number. Takes pedal into account.
* @param r Red intensity
* @param g Green intensity
* @param b Blue intensity
* @param noteNr Note number
* @author Daniël Schenk
* @date 2011-09-28
*/
void ledSingleColorUpdateLedOn(uint8_t r, uint8_t g, uint8_t b, uint8_t noteNr)
{
// 	ledsR[ledMapping[noteNr]]=notes[noteNr]*(r/255);
// 	ledsG[ledMapping[noteNr]]=notes[noteNr]*(g/255);
// 	ledsB[ledMapping[noteNr]]=notes[noteNr]*(b/255);
	ledsR[ledMapping[noteNr]] = (ledsR[ledMapping[noteNr]] < notes[noteNr]*(r/255)) ? notes[noteNr]*(r/255) : ledsR[ledMapping[noteNr]] ;
	ledsG[ledMapping[noteNr]] = (ledsG[ledMapping[noteNr]] < notes[noteNr]*(g/255)) ? notes[noteNr]*(g/255) : ledsG[ledMapping[noteNr]] ;
	ledsB[ledMapping[noteNr]] = (ledsB[ledMapping[noteNr]] < notes[noteNr]*(b/255)) ? notes[noteNr]*(b/255) : ledsB[ledMapping[noteNr]] ;
}

void ledSingleColorUpdateLedOff(uint8_t noteNr)
{
	ledsR[ledMapping[noteNr]] = 0;
	ledsG[ledMapping[noteNr]] = 0;
	ledsB[ledMapping[noteNr]] = 0;
}
/**
* This function writes intensity values for all LEDs into memory, according to current note velocities.
* @param r Red intensity. Pass -1 to keep current value.
* @param g Green intensity. Pass -1 to keep current value.
* @param b Blue intensity. Pass -1 to keep current value.
* @author Daniël Schenk
* @date 2011-09-28
*/
void ledSingleColorSetFull(int16_t r, int16_t g, int16_t b)
{
	for (int ledNr=0; ledNr<ledsConnected; ledNr++)
	{
		if (r >= 0)
		{
			ledsR[ledNr]=(uint8_t)r;
		}
		if (g >= 0)
		{
			ledsG[ledNr]=(uint8_t)g;
		}
		if (b >= 0)
		{
			ledsB[ledNr]=(uint8_t)b;
		}
	}

}
/**
* This function writes a single LED intensity into memory, according to the provided LED number.
* @param r Red intensity
* @param g Green intensity
* @param b Blue intensity
* @param ledNr LED number
* @author Daniël Schenk
* @date 2012-01-03
*/
void ledSingleColorSetLed(uint8_t r, uint8_t g, uint8_t b, uint8_t ledNr)
{
	ledsR[ledNr]=r;
	ledsG[ledNr]=g;
	ledsB[ledNr]=b;
}

/**
* This method is used for writing the next following byte of intensity values to the LED strip. Interrupt based, to free up processor time. Must be used as quickly as possible since previous written byte, because the LED strip updates the LEDs after 500-800 uS of clock inactivity. When the function is called for the first time, the first byte is written. Then the function completes, and keeps track of next LED number and color that needs to be written. Other tasks can be done on the MCU. The TX complete interrupt from the USART can be used to trigger the next function call, which then writes the next byte, notes the next number and color, and so on. When all colors of all LEDs have been written, writeStripComplete is written 1, and prohibits further action of this method. Also a 800 uS timer is started. During the 800 uS, nothing may be written to the strip, to make sure the LEDs are being updated with the new intensities. The timer complete interrupt can be used to reset writeStripComplete, allowing a new write action to the strip.
* @author Daniël Schenk
* @date 2011-09-29
*/
void ledWriteNextByte()
{
	static uint8_t currentLed=4;
	switch (ledWriteState)
	{
		case writeR:
			UDR1 = (uint8_t)ledsR[currentLed];
			ledWriteState = writeG;
			break;
		case writeG:
			UDR1 = (uint8_t)ledsG[currentLed];
			ledWriteState = writeB;
			break;
		case writeB:
			UDR1 = (uint8_t)ledsB[currentLed];
			
			if (currentLed==41)
			{
				ledWriteState = writeR;
				currentLed = 46;
				break;
			}
			
			if (currentLed==85)
			{
				currentLed=4;
				ledWriteState = pause;
				//writeStripComplete = 1;
				TCNT0 = 0; //Reset timer value
				TCCR0B = (1<<CS02|0<<CS01|0<<CS00); //Start timer, CLK/256
				TIMSK0 = (1<<OCIE0A); //Enable output compare match 1 interrupt
				OCR0A = 0xE9; //0xF9 is 800uS with 20MHz clock and 64 prescale
				break;
			}
			else
			{
				ledWriteState = writeR;
				currentLed++;
				break;
			}
		case render: //Not used
			ledRenderAfterEffects(ledMode);
			ledWriteState = pause;
			break;
		case pause:
			break;
		default:
			break;
	}
}

/**
* This method ends the pause period required for the LED strip to apply received values, and allows a new write cycle to start.
* @author Daniël Schenk
* @date 2011-12-?
*/
void ledEndPause(void)
{
	TCCR0B = (0<<CS02|0<<CS01|0<<CS00); //Stop timer
	//TCNT0 = 0; //Reset timer value
	//writeStripComplete = 0;
	ledWriteState = writeR;
	if(ledAutoWrite > 0)
	{
		ledWriteNextByte();
	}
}
/**
* This method is used for rendering LED effects after turning on (e.g. dimming slowly to zero). Designed for running at a fixed interval.
* @param mode Global LED effect mode.
* @author Daniël Schenk
* @date 2011-12-?
*/
void ledRenderAfterEffects(unsigned int mode)
{
	//static uint8_t ledsRcount[ledsProgrammed] = { 1 };
	//static uint8_t ledsGcount[ledsProgrammed] = { 1 };
	//static uint8_t ledsBcount[ledsProgrammed] = { 1 };
	#define tp1 100
	#define tp2 75
	#define tp3 50
	#define dr1 10
	#define dr2 5
	#define dr3 2
	#define dr4 1
	#define freqDiv 2	
	//Rendering LEDs happens in the same way for modes 8-14
	if(mode >= 9 && mode <= 14)
	{
		mode = 8;
	}
	uint8_t note, r, g, b;
	bool any_on;
	switch(mode)
	{
		case 8:
			for(int ledNr = 0; ledNr<ledsConnected; ledNr++)
			{
				if(ledsR[ledNr]>100)
					ledsR[ledNr] = ledsR[ledNr] - ledsR[ledNr]/100;
				else if(ledsR[ledNr]>0/* && (ledsRcount[ledNr] % freqDiv)==0*/)
				{
					ledsR[ledNr]--;
					//ledsRcount[ledNr]++;
				}				
				//else if(ledsR[ledNr]==0)
					//ledsRcount[ledNr] = 0;
				if(ledsG[ledNr]>100)
					ledsG[ledNr] = ledsG[ledNr] - ledsG[ledNr]/100;
				else if(ledsG[ledNr]>0)
					ledsG[ledNr]--;
				if(ledsB[ledNr]>100)
					ledsB[ledNr] = ledsB[ledNr] - ledsB[ledNr]/100;
				else if(ledsB[ledNr]>0)
					ledsB[ledNr]--;

// 				if(ledsR[ledNr]>tp1)
// 					ledsR[ledNr] = ledsR[ledNr] - dr1;
// 				else if(ledsR[ledNr]>tp2)
// 					ledsR[ledNr] = ledsR[ledNr] - dr2;
// 				else if(ledsR[ledNr]>tp3)
// 					ledsR[ledNr] = ledsR[ledNr] - dr3;
// 				else if(ledsR[ledNr]>0)
// 					ledsR[ledNr] = ledsR[ledNr] - dr4;
// 				if(ledsG[ledNr]>tp1)
// 					ledsG[ledNr] = ledsG[ledNr] - dr1;
// 				else if(ledsG[ledNr]>tp2)
// 					ledsG[ledNr] = ledsG[ledNr] - dr2;
// 				else if(ledsG[ledNr]>tp3)
// 					ledsG[ledNr] = ledsG[ledNr] - dr3;
// 				else if(ledsG[ledNr]>0)
// 					ledsG[ledNr] = ledsG[ledNr] - dr4;
// 				if(ledsB[ledNr]>tp1)
// 					ledsB[ledNr] = ledsB[ledNr] - dr1;
// 				else if(ledsB[ledNr]>tp2)
// 					ledsB[ledNr] = ledsB[ledNr] - dr2;
// 				else if(ledsB[ledNr]>tp3)
// 					ledsB[ledNr] = ledsB[ledNr] - dr3;
// 				else if(ledsB[ledNr]>0)
// 					ledsB[ledNr] = ledsB[ledNr] - dr4;
				
				
// 				if(ledsR[ledNr] > tp1)
// 					ledsR[ledNr] = ledsR[ledNr] - dr1;
// 				else if (ledsR[ledNr] <= tp1 && ledsR[ledNr] > 0)
// 					ledsR[ledNr] = ledsR[ledNr] - dr2;
// 				if(ledsG[ledNr] > tp1)
// 					ledsG[ledNr] = ledsG[ledNr] - dr1;
// 				else if (ledsG[ledNr] <= tp1 && ledsG[ledNr] > 0)
// 					ledsG[ledNr] = ledsG[ledNr] - dr2;
// 				if(ledsB[ledNr] > tp1)
// 					ledsB[ledNr] = ledsB[ledNr] - dr1;
// 				else if (ledsB[ledNr] <= tp1 && ledsB[ledNr] > 0)
// 					ledsB[ledNr] = ledsB[ledNr] - dr2;
			}
			break;
		case 52: /* Treasure intro */
			/* In this mode, every silent note gets a red background based on the 
			 * expression pedal position. The background is only enabled when any note
			 * is played, so first check if any note is played. */
			any_on = false;
			for(note = 0; note < 88; note++)
			{
				if(notes[note] > 0)
				{
					any_on = true;
					break;
				}
			}
			if(any_on)
			{
				/* Take expression as red intensity */
				r = midiExpression;
			}
			else
			{
				/* No background */
				r = 0;
			}
			g = 0;
			b = 0;
			for(note = 0; note < 88; note++)
			{
				if(notes[note] == 0)
				{
					/* This note is currently silent. Set it to the background color. */
					ledSingleColorSetLed(r, g, b, ledMapping[note]);
				}
			}
			break;
		default:
			break;
	}
}
/**
* This method is used for rendering a single LED according to a noteOn MIDI message being handled. Designed for being called from the MIDI handling routine.
* @param inputNote The note for which the corresponding LED needs to be set.
* @param mode Global LED effect mode.
* @author Daniël Schenk
* @date 2012-01-03
*/
void ledRenderFromNoteOn(unsigned char inputNote, unsigned int mode)
{
	//Turning LED on happens in the same way for some modes
	if((mode >= 2 && mode <= 14) || mode == 52 /* Treasure intro */)
	{
		mode = 1;
	}
	switch(mode)
	{
		static uint8_t mode51 = 0;
		case 1:
			ledSingleColorUpdateLedOn(rMax,gMax,bMax,inputNote);
			break;
		default:
			break;
		case 50: //Red and blue, determined by note number odd/even (Copyright!)
			if ((inputNote % 2) == 0)
			{
				ledSingleColorUpdateLedOn(rMax,0,0,inputNote);
			}
			else
			{
				ledSingleColorUpdateLedOn(0,0,bMax,inputNote);
			}
			break;
		case 51: //Red and blue, alternated from note to note (Copyright v2!)
			if (ledsR[ledMapping[inputNote]] == 0 && ledsB[ledMapping[inputNote]] == 0)
			{
				if (mode51 == 0)
				{
					ledSingleColorUpdateLedOn(rMax,0,0,inputNote);
					mode51 = 1;
				}
				else
				{
					ledSingleColorUpdateLedOn(0,0,bMax,inputNote);
					mode51 = 0;
				}
			}
			else if (ledsR[ledMapping[inputNote]] != 0)
			{
				ledSingleColorUpdateLedOn(rMax,0,0,inputNote);
			}
			else
			{
				ledSingleColorUpdateLedOn(0,0,bMax,inputNote);
			}
			
			break;
	}
// 	if(ledAutoWrite==0)
// 	{
// 		ledWriteNextByte();
// 	}
}
/**
* This method is used for rendering a single LED according to a noteOff MIDI message being handled. Designed for being called from the MIDI handling routine.
* @param inputNote The note for which the corresponding LED needs to be set.
* @param mode Global LED effect mode.
* @author Daniël Schenk
* @date 2012-01-03
*/
void ledRenderFromNoteOff(unsigned char inputNote, unsigned int mode)
{
	//Turning LED off happens in the same way for the first 7 modes
	if(mode >= 2 && mode <= 7)
	{
		mode = 1;
	}
	if((mode >= 8 && mode <= 14) || mode == 50/*Copyright!*/ || mode == 51/*Copyright!*/)
	{
		mode = 8;
	}
	switch(mode)
	{
		case 1:
			ledSingleColorUpdateLedOff(inputNote);
			break;
		case 8:
			if(midiSustain == 0)
				ledSingleColorUpdateLedOff(inputNote);
			break;
		default:
			break;
	}
// 	if(ledAutoWrite==0)
// 	{
// 		ledWriteNextByte();
// 	}
}

void ledRenderFromSustain(unsigned char mode, unsigned char sustain)
{
	if(mode >= 2 && mode <= 7) //Non-sustain modes
	{
		mode = 1;
	}
	if((mode >= 8 && mode <= 14) || mode == 50/*Copyright!*/ || mode == 51/*Copyright!*/) //Sustain modes
	{
		mode = 8;
	}
	switch(mode)
	{
		case 8: //Switch off LEDs at release of sustain pedal
			if(sustain == 0)
			{
				for(int noteNr = 0; noteNr<88; noteNr++)
				{
					if(notes[noteNr]==0)
						ledSingleColorSetLed(0,0,0,ledMapping[noteNr]);
				}
			}			
			break;
		default:
			break;
	}
}
/**
* This method can be used to start or stop the automatic writing to the LED strip.
* @param enable If greater than 0, automatic writing is enabled.
* @author Daniël Schenk
* @date 2012-01-02
*/
void ledSetAutoWrite(uint8_t enable)
{
	ledAutoWrite = enable;
	if(ledAutoWrite > 0)
	{
		ledWriteNextByte();
	}
}
/**
* This method can be used to change the LED effect mode, and sets the corresponding intensity value for each mode.
* @param modeNr The LED effect mode number.
* @author Daniël Schenk
* @date 2012-01-03
*/
void ledModeChange(unsigned int modeNr)
{
	ledMode = modeNr;
	//BV4513_writeNumber(ledMode);
	
	//Modes 8-14 have the same intensity settings as modes 1-7
	if(modeNr >= 8 && modeNr <= 14)
	{
		modeNr = modeNr - 7;
	}
	switch(modeNr)
	{
		case 1:
			rMax = ledMaxInt;
			gMax = 0;
			bMax = 0;
			break;
		case 2:
			rMax = 0;
			gMax = ledMaxInt;
			bMax = 0;
			break;
		case 3:
			rMax = 0;
			gMax = 0;
			bMax = ledMaxInt;
			break;
		case 4:
			rMax = ledMaxInt;
			gMax = ledMaxInt;
			bMax = 0;
			break;
		case 5:
			rMax = 0;
			gMax = ledMaxInt;
			bMax = ledMaxInt;
			break;
		case 6:
			rMax = ledMaxInt;
			gMax = 0;
			bMax = ledMaxInt;
			break;
		case 7:
			rMax = ledMaxInt;
			gMax = ledMaxInt;
			bMax = ledMaxInt;
			break;
		case 50: //Copyright!
			rMax = ledMaxInt;
			gMax = 0;
			bMax = ledMaxInt;
		case 51: //Copyright!
			rMax = ledMaxInt;
			gMax = 0;
			bMax = ledMaxInt;
		case 52: //Treasure intro!
			rMax = 0;
			gMax = 0;
			bMax = ledMaxInt;
			break;
		default:
			break;
	}
// 	#ifdef Debug
// 	ledSingleColorSetLed(rMax,gMax,bMax,1);
// 	_delay_ms(5000);
// 	ledSingleColorSetLed(0,0,0,1);
// 	#endif
}

void ledTestLoops()
{
	for(int i=0; i<ledsConnected; i++)
	{
		ledsR[i]=ledTestSetpoint;
		//ledWriteNextByte();
		_delay_ms(50);
	}
	for(int i=0; i<ledsConnected; i++)
	{
		ledsG[i]=ledTestSetpoint;
		//ledWriteNextByte();
		_delay_ms(50);
	}
	for(int i=0; i<ledsConnected; i++)
	{
		ledsB[i]=ledTestSetpoint;
		//ledWriteNextByte();
		_delay_ms(50);
	}
	ledTestSetpoint = (ledTestSetpoint>0)?  0 : setpoint_high;
}