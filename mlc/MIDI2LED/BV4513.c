/*
 * BV4513.c
 *
 * Created: 6-12-2011 19:55:01
 *  Author: Daniël
 */ 

#include "BV4513.h"
#include <avr/delay.h>
//#include "TWI_Master.h"

enum BV4513_writeStateEnum BV4513_writeState = start;
unsigned char BV4513_data[4];

void BV4513_init() 
{
	/*TWBR = 0x0C;
	TWDR = 0xFF;    // Default content = SDA released.
	TWCR = (1<<TWEN)|                                 // Enable TWI-interface and release TWI pins.
		(1<<TWIE)|(1<<TWINT)|                      // Enable Interupt.*/
	TWI_Master_Initialise();
	BV4513_clear();
}

char nthdigit(int x, int n)
{
    static int powersof10[4] = {1, 10, 100, 1000};
    return ((x / powersof10[n]) % 10) + '0';
}

/*
void BV4513_writeNextByte() 
{
	switch(BV4513_writeState)
	{
		case start:
			
	}
}
*/

void BV4513_writeDigit(unsigned char number, unsigned char digit)
{
	unsigned char data[4] = {BV4513_addr, 4, digit, number};
	TWI_Start_Transceiver_With_Data(data, 4);
}

void BV4513_writeNumber(int number)
{
	unsigned char data[4] = {BV4513_addr, 4, 0, 0};
	for(int i=0; i<3; i++)
	{ 
		data[2] = (unsigned char)i;
		data[3] = (unsigned char)nthdigit(number, i);
		TWI_Start_Transceiver_With_Data(data, 4);
	}
}

void BV4513_clear()
{
	unsigned char data[2] = {BV4513_addr, 2};
	TWI_Start_Transceiver_With_Data(data, 2);
}

void BV4513_setDecimalPoint(unsigned char digit, unsigned char enable)
{
	unsigned char data[4] = {BV4513_addr, 5, digit, enable};
	TWI_Start_Transceiver_With_Data(data, 4);
}
