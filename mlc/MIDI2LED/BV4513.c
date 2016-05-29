/*
 * BV4513.c
 *
 * Created: 6-12-2011 19:55:01
 *  Author: Daniël
 */ 

#include "BV4513.h"
#include "globals.h"

#include <util/delay.h>

void BV4513_init() 
{
	/*TWBR = 0x0C;
	TWDR = 0xFF;    // Default content = SDA released.
	TWCR = (1<<TWEN)|                                 // Enable TWI-interface and release TWI pins.
		(1<<TWIE)|(1<<TWINT)|                      // Enable Interupt.*/
	TWI_Master_Initialise();
	BV4513_clear();
}

/** @brief Write a digit to the display
 * 
 * Digit positions on the display when front-facing:
 * _________________________
 * |     |     |     |     |
 * |  0  |  1  |  2  |  3  |
 * |    .|    .|    .|    .|
 * -------------------------
 * @param[in]    val    Digit value to write
 * @param[in]    pos    Digit position to write
 */
void BV4513_writeDigit(unsigned char val, unsigned char pos)
{
	unsigned char data[4] = {BV4513_addr, 4, pos, val};
	TWI_Start_Transceiver_With_Data(data, 4);
}

void BV4513_writeNumber(int number)
{
	/* Least significant digit is at pos 3 on the display */
	for(int8_t pos=3; pos>=0; pos--)
	{ 
		uint8_t digit_val = number % 10;
		BV4513_writeDigit(digit_val, pos);
		number /= 10;
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
