/*
 * BV4513.c
 *
 * Created: 6-12-2011 19:55:01
 *  Author: Daniël
 */

#include "BV4513.h"
#include "globals.h"

#include <util/delay.h>
#include <avr/pgmspace.h>
#include <string.h>
#include <stdbool.h>

static const char char_table_start = '-';
static const char char_table[] PROGMEM = {
	0x40, /* - */
	0x80, /* . */
	0x00, /* / */
	0x3f, /* 0 */
	0x06, /* 1 */
	0x5b, /* 2 */
	0x4f, /* 3 */
	0x66, /* 4 */
	0x6d, /* 5 */
	0x7d, /* 6 */
	0x07, /* 7 */
	0x7f, /* 8 */
	0x6f, /* 9 */
	0x00, /* : */
	0x00, /* ; */
	0x00, /* < */
	0x48, /* = */
	0x00, /* > */
	0x00, /* ? */
	0x00, /* @ */
	0x77, /* A */
	0x7c, /* B */
	0x39, /* C */
	0x5e, /* D */
	0x79, /* E */
	0x71, /* F */
	0x00, /* G */
	0x76, /* H */
	0x30, /* I */
	0x0e, /* J */
	0x00, /* K */
	0x38, /* L */
	0x00, /* M */
	0x37, /* N */
	0x5c, /* O */
	0x73, /* P */
	0x00, /* Q */
	0x50, /* R */
	0x00, /* S */
	0x00, /* T */
	0x3e, /* U */
	0x1c, /* V */
	0x00, /* W */
	0x00, /* X */
	0x00, /* Y */
	0x00, /* Z */
	0x00, /* [ */
	0x00, /* \ */
	0x00, /* ] */
	0x00, /* ^ */
	0x08, /* _ */
	0x00, /* ` */
	0x5f, /* a */
	0x7c, /* b */
	0x58, /* c */
	0x5e, /* d */
	0x79, /* e */
	0x71, /* f */
	0x00, /* g */
	0x74, /* h */
	0x30, /* i */
	0x0e, /* j */
	0x00, /* k */
	0x38, /* l */
	0x00, /* m */
	0x54, /* n */
	0x5c, /* o */
	0x73, /* p */
	0x00, /* q */
	0x50, /* r */
	0x00, /* s */
	0x00, /* t */
	0x3e, /* u */
	0x1c, /* v */
	0x00, /* w */
	0x00, /* x */
	0x00, /* y */
	0x00, /* z */
};

void BV4513_init()
{
	/*TWBR = 0x0C;
	TWDR = 0xFF;    // Default content = SDA released.
	TWCR = (1<<TWEN)|                                 // Enable TWI-interface and release TWI pins.
		(1<<TWIE)|(1<<TWINT)|                      // Enable Interupt.*/
	TWI_Master_Initialise();
	BV4513_clear();

	/* Write back maximum brightness which is also the power-on default.
	 * This prevents a different brightness value to stay in the display when
	 * our software has reset. TODO: actually we should fully reset the display,
	 * but this doesn't work yet! */
	BV4513_setBrightness(25);
}

void BV4513_writeSegments(unsigned char segments, unsigned char pos)
{
	unsigned char data[4] = {BV4513_addr, 3, pos, segments};
	TWI_Start_Transceiver_With_Data(data, sizeof(data));
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
	TWI_Start_Transceiver_With_Data(data, sizeof(data));
}

void BV4513_writeNumber(int number)
{
	BV4513_clear();
	/* Least significant digit is at pos 3 on the display */
	for(int8_t pos=3; pos>=0; pos--)
	{
		uint8_t digit_val = number % 10;
		BV4513_writeDigit(digit_val, pos);
		number /= 10;
	}
}

static void writeString(const char * s, int pos, bool progmem)
{
	BV4513_clear();
	int curr_pos = pos;
	for(const char *p = s; *p != 0; p++)
	{
		if(curr_pos > 3)
			break; /* Reached end of display */

		char c;
		if (progmem)
			c = pgm_read_byte(p);
		else
			c = *p;
		if(c == '.')
		{
			/* Special feature: enable dot on just-written position */
			if(curr_pos-1 >= 0 && curr_pos-1 >= pos)
				BV4513_setDecimalPoint(curr_pos-1, 1);
			continue;
		}
		else if(c >= char_table_start && c < char_table_start + sizeof(char_table))
		{
			/* Character is in the char table */
			c = pgm_read_byte(&char_table[c-char_table_start]);
		}
		else
		{
			/* Fallback for non-supported chars is empty */
			c = 0;
		}

		BV4513_writeSegments(c, curr_pos);
		curr_pos++;
	}
}

void BV4513_writeString(const char * s, int pos)
{
	writeString(s, pos, false);
}

void BV4513_writeString_P(const char * s, int pos)
{
	writeString(s, pos, true);
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

void BV4513_reset()
{
	unsigned char data[] = {BV4513_addr, 0x95};
	TWI_Start_Transceiver_With_Data(data, sizeof(data));
}

void BV4513_setBrightness(unsigned char value)
{
	if(value > 25)
		value = 25;
	unsigned char data[] = {BV4513_addr, 1, value};
	TWI_Start_Transceiver_With_Data(data, sizeof(data));
}
