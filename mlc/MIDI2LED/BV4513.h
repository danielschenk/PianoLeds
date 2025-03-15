/*
 * BV4513.h
 *
 * Created: 6-12-2011 19:55:25
 *  Author: Daniël
 */


#ifndef BV4513_H_
#define BV4513_H_

#include "TWI_Master.h"

#define BV4513_addr 0x62 //!< I²C address of the display

void BV4513_writeNumber(int number);
void BV4513_init();
void BV4513_clear();
//void BV4513_writeNextByte();
void BV4513_writeDigit(unsigned char val, unsigned char pos);
void BV4513_writeString(const char * s, int pos);
void BV4513_writeString_P(const char * s, int pos);
void BV4513_setDecimalPoint(unsigned char digit, unsigned char enable);
void BV4513_setBrightness(unsigned char value);
void BV4513_reset();
enum BV4513_writeStateEnum
{
	start,
	stop,
	addr_rw,
	command,
	data
};

#endif /* BV4513_H_ */

