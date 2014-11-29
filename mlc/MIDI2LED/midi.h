/**
* @file midi.h
* @brief MIDI related definitions
* 
*
* @author Daniël Schenk
*
* @date 2011-09-28
*/

#ifndef MIDI_H_
#define MIDI_H_
#include <inttypes.h>
#include "ledstrip.h"


//#define midiLogEnabled
#define midiLogSize 200
#define midiLowestNote 21
#define midiHighestNote 108

enum midiReceiveStateEnum
{
	statusByte,
	noteNrOn,
	noteNrOff,
	velocityOn,
	velocityOff,
	progChange,
	controlChange,
	controlValue,
	skip,
};

extern unsigned char notes[88];
extern unsigned char midiSustain;

void midiHandleByte();
void midiInit();
void midiUSART0Init();
void midiIndicator(unsigned char enable);

void midiLogByte(unsigned char input);
uint8_t midiNoteNrMapped(uint8_t note);

#endif /* MIDI_H_ */
