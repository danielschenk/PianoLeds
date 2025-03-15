/**
* @file ledstrip.h
* @brief LED strip related definitions
*
*
* @author Daniël Schenk
*
* @date 2011-09-28
*/

#ifndef LEDSTRIP_H_
#define LEDSTRIP_H_

#define ledsProgrammed 88
#define ledsConnected 82 //!< Used in development phase for testing with less LED strip elements
#define ledBaud 2000000 //!< Ledstrip data rate
#define setpoint_high 255 //!< For debugging purposes
#define ledMaxInt 255 //!< Global maximum intensity

#include <inttypes.h>

enum ledWriteStateEnum
{
	writeR,
	writeG,
	writeB,
	pause,
	render
};

void ledInit();
void ledSingleColorUpdateFull(uint8_t r, uint8_t g, uint8_t b);
void ledSingleColorUpdateLedOn(uint8_t r, uint8_t g, uint8_t b, uint8_t noteNr);
void ledCreateMapping();
void ledInitUSART1SPI(long baud);
void ledWriteNextByte();
void ledEndPause(void);
void ledRenderAfterEffects(unsigned int mode);
void ledRenderFromNoteOn(unsigned char inputNote, unsigned int mode);
void ledRenderFromNoteOff(unsigned char inputNote, unsigned int mode);
void ledSingleColorSetLed(uint8_t r, uint8_t g, uint8_t b, uint8_t ledNr);
void ledSingleColorSetFull(int16_t r, int16_t g, int16_t b);
void ledRenderFromSustain(unsigned char mode, unsigned char sustain);
void ledSingleColorUpdateLedOff(uint8_t noteNr);

#endif
