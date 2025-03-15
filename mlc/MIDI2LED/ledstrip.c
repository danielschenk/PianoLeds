/**
* @file ledstrip.c
* @brief LED strip related functions
*
*
* @author Daniël Schenk
*
* @date 2011-09-28
*/

#include "Model/ConfigurationModel.h"
#include "Common/TimerService.h"
#include "globals.h"
#include "ledstrip.h"
#include "BV4513.h"
#include "midi.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>

#define MAX_INTENSITY UINT8_MAX
#define NUM_ELEMENTS(array) (sizeof(array) / sizeof(array[0]))

/**
 * Callback function for preset change events, triggered from model.
 */
static void CurrentPresetChangedCallback(void *arg);

typedef struct
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
} Color;

typedef enum
{
	MODE_START_NO_SUSTAIN = 1,
	MODE_RED = 1,
	MODE_GREEN = 2,
	MODE_BLUE = 3,
	MODE_YELLOW = 4,
	MODE_CYAN = 5,
	MODE_MAGENTA = 6,
	MODE_WHITE = 7,
	MODE_END_NO_SUSTAIN = 8,

	MODE_START_SUSTAIN = 8,
	MODE_RED_SUSTAIN = 8,
	MODE_GREEN_SUSTAIN = 9,
	MODE_BLUE_SUSTAIN = 10,
	MODE_YELLOW_SUSTAIN = 11,
	MODE_CYAN_SUSTAIN = 12,
	MODE_MAGENTA_SUSTAIN = 13,
	MODE_WHITE_SUSTAIN = 14,
	MODE_END_SUSTAIN = 15,

	// specials
	MODE_COPYRIGHT = 50,
	MODE_COPYRIGHT_V2 = 51,
	MODE_TREASURE_INTRO = 52,
	MODE_PETER_GUNN = 53,
	MODE_MULTICOLOR = 54,

	// tests
	MODE_FULL_STRIP_COLOR_CYCLE_TEST,
} Mode;

static const Color ledTestColors[] = {
	{MAX_INTENSITY, 0, 0},
	{0, MAX_INTENSITY, 0},
	{0, 0, MAX_INTENSITY},
	{MAX_INTENSITY, MAX_INTENSITY, 0},
	{MAX_INTENSITY, 0, MAX_INTENSITY},
	{0, MAX_INTENSITY, MAX_INTENSITY},
	{MAX_INTENSITY, MAX_INTENSITY, MAX_INTENSITY},
};

static const Color* ledTestColor = ledTestColors;

static void LedTestTimerCallback(TimerId_t unused)
{
	const Color* end = &ledTestColors[NUM_ELEMENTS(ledTestColors)];
	if (++ledTestColor >= end)
		ledTestColor = ledTestColors;
}

/**
* This method can be used to change the LED effect mode, and sets the corresponding intensity value for each mode.
* @param modeNr The LED effect mode number.
*/
static void ledModeChange(unsigned int modeNr);

static uint8_t ledMapping[88]; //!<Note number to LED number mapping. mapping[noteNr]==ledNr

static enum ledWriteStateEnum ledWriteState = writeR;

static unsigned char rMax; //!< Red intensity maximum (varies according to effect mode)
static unsigned char gMax; //!< Green intensity maximum (varies according to effect mode)
static unsigned char bMax; //!< Blue intensity maximum (varies according to effect mode)

static uint8_t ledsR[ledsProgrammed]; //!<Red intensity values
static uint8_t ledsG[ledsProgrammed]; //!<Green intensity values
static uint8_t ledsB[ledsProgrammed]; //!<Blue intensity values

static Color backgroundColor = {0, 0, 0};

static const Color multiColorColors[] = {
	{MAX_INTENSITY, 0, 0},
	{0, MAX_INTENSITY, 0},
	{0, 0, MAX_INTENSITY},
	{MAX_INTENSITY, MAX_INTENSITY, 0},
	{MAX_INTENSITY, 0, MAX_INTENSITY},
	{0, MAX_INTENSITY, MAX_INTENSITY},
};

/**
* This method writes the mapping of note numbers to LED numbers in memory.
* @author Daniël Schenk
* @date 2011-09-29
*/
static void ledCreateMapping()
{
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
}

/**
* This method configures USART1 in SPI mode for LED strip communication. Based on example from ATmega164P data sheet.
* @author Daniël Schenk
* @param baud Desired baud rate
* @date 2011-09-29
*/
static void ledInitUSART1SPI(long baud)
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
	ledWriteNextByte();

    ConfigurationModel_SubscribeCurrentPreset(CurrentPresetChangedCallback);
    /* Make sure configuration is done for initial preset */
	ledModeChange(ConfigurationModel_GetCurrentPreset());

	TimerService_Create(2000, LedTestTimerCallback, true);
}

/**
 * Calculate LED intensity from a velocity, taking factor into account
 *
 * @param velocity  The note velocity
 * @param factor    The factor, 0 is 0%, 255 is 100%
 * @return          The calculated LED intensity
 */
static uint8_t velocityToIntensity(uint8_t velocity, uint8_t factor)
{
    /* MIDI velocity has range 0-127. LEDs have range 0-255. Upscale velocity first */
    uint8_t intensity = velocity * 2;
    /* Apply factor */
    intensity *= (factor / 255);

    return intensity;
}

/**
* This function writes intensity values for all LEDs into memory, according to current note velocities.
* @param r Red intensity
* @param g Green intensity
* @param b Blue intensity
*/
void ledSingleColorUpdateFull(uint8_t r, uint8_t g, uint8_t b)
{
	for (int noteNr=0; noteNr<88; noteNr++)
	{
        uint8_t ledNumber = ledMapping[noteNr];
		ledsR[ledNumber] = velocityToIntensity(notes[noteNr], r);
		ledsG[ledNumber] = velocityToIntensity(notes[noteNr], g);
		ledsB[ledNumber] = velocityToIntensity(notes[noteNr], b);
	}
}

/**
 * Overwrite the actual intensity at the given address with the new intensity if the new intensity is higher
 *
 * @param actualIntensity   Pointer to the actual intensity
 * @param newIntensity      The new intensity
 */
static void applyNewIntensityIfHigher(uint8_t* actualIntensity, uint8_t newIntensity)
{
    if(newIntensity > *actualIntensity)
    {
        *actualIntensity = newIntensity;
    }
}

/**
* This function writes a single LED intensity into memory, according to the current note velocity of the provided note number. Takes pedal into account.
* @param r Red intensity
* @param g Green intensity
* @param b Blue intensity
* @param noteNr Note number
*/
void ledSingleColorUpdateLedOn(uint8_t r, uint8_t g, uint8_t b, uint8_t noteNr)
{
    uint8_t velocity = notes[noteNr];
    uint8_t ledNumber = ledMapping[noteNr];
    applyNewIntensityIfHigher(&ledsR[ledNumber], velocityToIntensity(velocity, r));
    applyNewIntensityIfHigher(&ledsG[ledNumber], velocityToIntensity(velocity, g));
    applyNewIntensityIfHigher(&ledsB[ledNumber], velocityToIntensity(velocity, b));
}

static void ledSingleColorUpdateLedOnMax(uint8_t noteNr)
{
    uint8_t ledNumber = ledMapping[noteNr];
    applyNewIntensityIfHigher(&ledsR[ledNumber], rMax);
    applyNewIntensityIfHigher(&ledsG[ledNumber], gMax);
    applyNewIntensityIfHigher(&ledsB[ledNumber], bMax);
}

static void multicolorLedOn(uint8_t noteNr)
{
	uint8_t velocity = notes[noteNr];
    uint8_t ledNumber = ledMapping[noteNr];
	static const Color* nextColor = multiColorColors;

	ledsR[ledNumber] = velocityToIntensity(velocity, nextColor->r);
	ledsG[ledNumber] = velocityToIntensity(velocity, nextColor->g);
	ledsB[ledNumber] = velocityToIntensity(velocity, nextColor->b);

	const Color* end = &multiColorColors[NUM_ELEMENTS(multiColorColors)];
	if (++nextColor >= end)
		nextColor = multiColorColors;
}

void ledSingleColorUpdateLedOff(uint8_t noteNr)
{
	ledsR[ledMapping[noteNr]] = backgroundColor.r;
	ledsG[ledMapping[noteNr]] = backgroundColor.g;
	ledsB[ledMapping[noteNr]] = backgroundColor.b;
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
	for (int ledNr=0; ledNr<ledsProgrammed; ledNr++)
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

static void setBackgroundColor(uint8_t r, uint8_t g, uint8_t b)
{
	if (r == backgroundColor.r && g == backgroundColor.g && b == backgroundColor.b)
		return;
	backgroundColor.r = r;
	backgroundColor.g = g;
	backgroundColor.b = b;
	ledSingleColorSetFull(r, g, b);
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
			ledRenderAfterEffects(ConfigurationModel_GetCurrentPreset());
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
}
/**
* This method is used for rendering LED effects after turning on (e.g. dimming slowly to zero). Designed for running at a fixed interval.
* @param mode Global LED effect mode.
* @author Daniël Schenk
* @date 2011-12-?
*/
void ledRenderAfterEffects(unsigned int mode)
{
	#define freqDiv 2
	//Rendering LEDs happens in the same way for modes 8-14
	if((mode >= MODE_START_SUSTAIN && mode < MODE_END_SUSTAIN)
		|| mode == MODE_MULTICOLOR)
	{
		mode = MODE_START_SUSTAIN;
	}
	uint8_t note, r, g, b;
	bool any_on;
	switch(mode)
	{
		case MODE_FULL_STRIP_COLOR_CYCLE_TEST:
			ledSingleColorSetFull(ledTestColor->r, ledTestColor->g, ledTestColor->b);
			break;
		case MODE_START_SUSTAIN:
			for (int ledNr = 0; ledNr<ledsProgrammed; ledNr++)
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
			}
			break;
		case MODE_TREASURE_INTRO:
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
	if (mode == MODE_PETER_GUNN)
	{
		// first note should turn on background
		setBackgroundColor(0, 0, 50);
	}

	//Turning LED on happens in the same way for some modes
	if((mode >= MODE_START_NO_SUSTAIN && mode < MODE_END_SUSTAIN) || mode == MODE_TREASURE_INTRO)
	{
		mode = MODE_START_NO_SUSTAIN;
	}
	switch(mode)
	{
		static uint8_t mode51 = 0;
		case MODE_START_NO_SUSTAIN:
			ledSingleColorUpdateLedOn(rMax,gMax,bMax,inputNote);
			break;
		case MODE_COPYRIGHT: //Red and blue, determined by note number odd/even
			if ((inputNote % 2) == 0)
			{
				ledSingleColorUpdateLedOn(rMax,0,0,inputNote);
			}
			else
			{
				ledSingleColorUpdateLedOn(0,0,bMax,inputNote);
			}
			break;
		case MODE_COPYRIGHT_V2: //Red and blue, alternated from note to note
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
		case MODE_PETER_GUNN:
			ledSingleColorUpdateLedOnMax(inputNote);
			break;
		case MODE_MULTICOLOR:
			multicolorLedOn(inputNote);
			break;
		default:
			break;
	}
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
	//Turning LED off happens in the same way for the first 7 modes and some more
	if((mode >= MODE_START_NO_SUSTAIN && mode < MODE_END_NO_SUSTAIN) || mode == MODE_PETER_GUNN)
	{
		mode = MODE_START_NO_SUSTAIN;
	}
	if((mode >= MODE_START_SUSTAIN && mode < MODE_END_SUSTAIN)
		|| mode == MODE_COPYRIGHT || mode == MODE_COPYRIGHT_V2 || mode == MODE_MULTICOLOR)
	{
		mode = MODE_START_SUSTAIN;
	}
	switch(mode)
	{
		case MODE_START_NO_SUSTAIN:
			ledSingleColorUpdateLedOff(inputNote);
			break;
		case MODE_START_SUSTAIN:
			if(midiSustain == 0)
				ledSingleColorUpdateLedOff(inputNote);
			break;
		default:
			break;
	}
}

void ledRenderFromSustain(unsigned char mode, unsigned char sustain)
{
	if(mode >= MODE_START_NO_SUSTAIN && mode < MODE_END_NO_SUSTAIN) //Non-sustain modes
	{
		mode = MODE_START_NO_SUSTAIN;
	}
	if((mode >= MODE_START_SUSTAIN && mode < MODE_END_SUSTAIN)
		|| mode == MODE_COPYRIGHT || mode == MODE_COPYRIGHT_V2 || mode == MODE_MULTICOLOR) //Sustain modes
	{
		mode = MODE_START_SUSTAIN;
	}
	switch(mode)
	{
		case MODE_START_SUSTAIN: //Switch off LEDs at release of sustain pedal
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

static void ledModeChange(unsigned int modeNr)
{
	//Modes 8-14 have the same intensity settings as modes 1-7
	if(modeNr >= MODE_START_SUSTAIN && modeNr < MODE_END_SUSTAIN)
	{
		modeNr = modeNr - (MODE_END_SUSTAIN - MODE_START_SUSTAIN);
	}
	if (modeNr != MODE_PETER_GUNN)
	{
		setBackgroundColor(0, 0, 0);
	}
	switch(modeNr)
	{
		case MODE_RED:
			rMax = ledMaxInt;
			gMax = 0;
			bMax = 0;
			break;
		case MODE_GREEN:
			rMax = 0;
			gMax = ledMaxInt;
			bMax = 0;
			break;
		case MODE_BLUE:
			rMax = 0;
			gMax = 0;
			bMax = ledMaxInt;
			break;
		case MODE_YELLOW:
			rMax = ledMaxInt;
			gMax = ledMaxInt;
			bMax = 0;
			break;
		case MODE_CYAN:
			rMax = 0;
			gMax = ledMaxInt;
			bMax = ledMaxInt;
			break;
		case MODE_MAGENTA:
			rMax = ledMaxInt;
			gMax = 0;
			bMax = ledMaxInt;
			break;
		case MODE_WHITE:
			rMax = ledMaxInt;
			gMax = ledMaxInt;
			bMax = ledMaxInt;
			break;
		case MODE_COPYRIGHT:
			rMax = ledMaxInt;
			gMax = 0;
			bMax = ledMaxInt;
		case MODE_COPYRIGHT_V2:
			rMax = ledMaxInt;
			gMax = 0;
			bMax = ledMaxInt;
		case MODE_TREASURE_INTRO:
			rMax = 0;
			gMax = 0;
			bMax = ledMaxInt;
			break;
		case MODE_PETER_GUNN:
			rMax = ledMaxInt;
			gMax = ledMaxInt;
			bMax = ledMaxInt;
			break;
		default:
			break;
	}
}

static void CurrentPresetChangedCallback(void *arg)
{
    uint8_t newPresetNumber = *(uint8_t *)arg;
    ledModeChange(newPresetNumber);
}
