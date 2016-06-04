/**﻿
* @file midi.c
* @brief MIDI related definitions
* 
*
* @author Daniël Schenk
*
* @date 2011-09-28
*/

#include "globals.h"
#include "midi.h"
#include "ledstrip.h"
#include "BV4513.h"
#include <avr/io.h>

unsigned char notes[88]; //!<Note velocity values
unsigned char notesRelease[88]; //!<Note release velocity values
unsigned char midiSustain; //!<Current value of sustain pedal

extern uint8_t ledsR[ledsProgrammed]; //!<Red intensity values
extern uint8_t ledsG[ledsProgrammed]; //!<Green intensity values
extern uint8_t ledsB[ledsProgrammed]; //!<Blue intensity values

unsigned char midiIndicatorSet = 0;

static unsigned char midiChannel;
volatile unsigned int midiErrorCount = 0;

enum midiReceiveStateEnum midiReceiveState = statusByte;

#ifdef midiLogEnabled
unsigned char midiBytes[midiLogSize];
unsigned int midiLogCount = 0;
#endif

//int counter = 0;


/**
* This method initializes the MIDI interface.
* @author Daniël Schenk
* @date 2011-09-30
*/
void midiInit()
{
	//midiChannel = (PORTA && 0x0F);
	midiChannel = 0;
	midiUSART0Init();
}
/**
* This method initializes USART0 for MIDI reception.
* @author Daniël Schenk
* @date 2011-09-30
*/
void midiUSART0Init()
{
	UCSR0B = (1<<RXCIE0 | 0<<TXCIE0 | 0<<UDRIE0 | 1<<RXEN0 | 0<<TXEN0 | 0<<UCSZ02);
	UCSR0C = (0<<UMSEL00 | 0<<UMSEL01 | 0<<UPM00 | 0<<UPM01 | 0<<USBS0 | 1<<UCSZ01 | 1<<UCSZ00);
	UBRR0 = (F_CPU/500000)-1;
}
/**
* This method is designed for the interrupt based handling of MIDI bytes. It contains a state machine and therefore requires corresponding global variables.
* @author Daniël Schenk
* @date 2011-12-07
*/
void midiHandleByte()
{
	static unsigned char currentParam; //!<Current note or controller number being handled
	
	//Save received byte from USART0
	unsigned char midiReceiveBuffer = UDR0; //!<To empty the USART receive register and save MIDI byte for use
	
	#ifdef midiLogEnabled
	midiLogByte(midiReceiveBuffer); //Record received byte for debugging purposes
	#endif
	
	//Extract the nibbles from received byte
	volatile unsigned char midiLowerNibble = midiReceiveBuffer & 0x0F; //!<Lower nibble of received MIDI byte
	volatile unsigned char midiUpperNibble = (midiReceiveBuffer & 0xF0)/16; //!<Upper nibble of received MIDI byte
	
	if((UCSR0A & (1<<FE0|1<<DOR0)) == 0) //If received without errors
	{
		//To get out of the 'skip' state when a new status byte arrives
		if(midiUpperNibble > 7)
		{
			midiReceiveState = statusByte;
		}
	}
	else
	{
		midiReceiveState = skip; //Do nothing with incorrectly received data
		UCSR0A = (UCSR0A & (~(1<<FE0|1<<DOR0))); //Clear any errors
		midiErrorCount++;
	}
	
	//unsigned char dummy = 0;
	
	
	
	switch(midiReceiveState)
	{
		case statusByte:
			if(midiLowerNibble != midiChannel) //When MIDI channel doesn't match
			{
				midiReceiveState = skip; //Do nothing
				break;
			}
			
			#if BUILD_DISPLAY
			if (midiReceiveState != skip)
			{
				//midiIndicator(1);
			}
			#endif
			
			switch(midiUpperNibble) //Determine what type of message is being received
			{
				case 0x09: //NoteOn message
					midiReceiveState = noteNrOn;
					break;
				case 0x08: //NoteOff message
					midiReceiveState = noteNrOff;
					break;
				case 0x0C: //Program change message
					midiReceiveState = progChange;
					break;
				case 0x0B: //Control change message
					midiReceiveState = controlChange;
					break;
				default: //In case of unknown or unimplemented status code
					midiReceiveState = skip; //Do nothing
					break;
			}
			break; //Break case statusByte (overlapping)
		case noteNrOn:
			currentParam = midiReceiveBuffer-midiLowestNote;
			midiReceiveState = velocityOn;
			break;
		case velocityOn:
			if(!midiNoteNrMapped(currentParam))
				break;
			notes[currentParam] = midiReceiveBuffer;
			ledRenderFromNoteOn(currentParam, ledMode);
			midiReceiveState = skip; //Further data is useless
			break;
		case noteNrOff:
			currentParam = midiReceiveBuffer-midiLowestNote;
			midiReceiveState = velocityOff;
			break;
		case velocityOff:
			if(!midiNoteNrMapped(currentParam))
				break;
			notes[currentParam] = 0; //Note needs to be turned off
			notesRelease[currentParam] = midiReceiveBuffer; //Save release velocity for later use
			ledRenderFromNoteOff(currentParam, ledMode);
			midiReceiveState = skip; //Further data is useless
			break;
		case progChange:
			ledModeChange(midiReceiveBuffer);
			midiReceiveState = skip;
			break;
		case controlChange:
			currentParam = midiReceiveBuffer; //Save controller number for next state
			midiReceiveState = controlValue;
			break;
		case controlValue:
			switch(currentParam) //Determine what variable has to be changed according to received controller number
			{
				case 0x40: //Sustain pedal
					midiSustain = midiReceiveBuffer;
					break;
				default:
					break;
				case 9: //Drawbar 1
					if (ledMode == 100)
					{
						ledSingleColorSetFull((midiReceiveBuffer*2), -1, -1);
					}
					break;
				case 14: //Drawbar 2
					if (ledMode == 100)
					{
						ledSingleColorSetFull(-1, (midiReceiveBuffer*2), -1);
					}
					break;
				case 15: //Drawbar 3
					if (ledMode == 100)
					{
						ledSingleColorSetFull(-1, -1, (midiReceiveBuffer*2));
					}
					break;
				case 16: //Drawbar 4
					if (ledMode == 100)
					{
						//max = 2*midiReceiveBuffer;
// 						for (int ledNr=0; ledNr<ledsConnected; ledNr++)
// 						{
// 							ledsR[ledNr]=ledsR[ledNr]*(max/255);
// 							ledsG[ledNr]=ledsR[ledNr]*(max/255);
// 							ledsB[ledNr]=ledsR[ledNr]*(max/255);
// 						}
					}
			}
			ledRenderFromSustain(ledMode, midiSustain);
			midiReceiveState = skip;
			break;
		case skip:
			break; //This case can be escaped only by arrival of a new status byte
		default:
			break;
	}
}

// void midiDisplayNote()
// {
// 	BV4513_writeNumber(currentNote-21);
// }

#ifdef midiLogEnabled
void midiLogByte(unsigned char input)
{
	midiBytes[midiLogCount] = input;
	
	if(midiLogCount>midiLogSize-1)
	{
		midiLogCount = 0;
	}
	else
	{
		midiLogCount++;
	}
}
#endif

uint8_t midiNoteNrMapped(uint8_t note)
{
	if (note >= 0 && note <= 87)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void midiIndicator(unsigned char enable)
{
	midiIndicatorSet = enable;
	BV4513_setDecimalPoint(2, enable);
}