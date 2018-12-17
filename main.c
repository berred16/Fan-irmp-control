/*----------------------------------------------------------------------------------
 * Copyright (C) 2018 youtube: berred
 *
 *This program is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.
 *
 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.
 *
 *You should have received a copy of the GNU General Public License
 *along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *---------------------------------------------------------------------------------
 *
 * Project: Control an old Fan using iR Control (based on irmp project)
 * By youtube berred
 * Build up: Tiny 84 with 7-Segment display, relays and iR sensor
 * 2018-12-08 q&d software
 * 
 * Portpins: 
 * Pin13: PA0 - SEG G
 * Pin12: PA1 - Seg F
 * Pin11: PA2 - Seg A
 * Pin10: PA3 - Seg D
 * Pin9 : PA4 - Seg C
 * Pin8 : PA5 - Seg B
 * Pin7 : PA6 - Seg E 
 * Pin6 : PA7 - iR Sensor (set in irmpconfig.h)
 * Pin2 : PB0 - Relay 1
 * Pin3 : PB1 - Relay 2
 * Pin4 : PB2 - Relay 3
 *---------------------------------------------------------------------------------
 */
 
#include "irmp.h"

#define F_CPU 8000000

// Some variables ...
int sec, min, i, speed, i_tim; 
int tim;

// 7 segment and relay symbolic constant
#define SEG_A 4
#define SEG_B 32
#define SEG_C 16
#define SEG_D 8
#define SEG_E 64
#define SEG_F 2
#define SEG_G 1
#define SEG_OFF 0
#define SEG_PORT PORTA

#define RELAY1 1
#define RELAY2 2
#define RELAY3 4
#define RELAY0 0
#define RELAY_PORT PORTB

// define buttons of the iR remote control
#define ZERO  0x0016
#define ONE   0x000C
#define TWO   0x0018
#define THREE 0x005E
#define FOUR  0x0008
#define FIVE  0x001C
#define SIX   0x005A
#define SEVEN 0x0042
#define EIGHT 0x0052
#define NINE  0x004A
#define TIMER 0x0044
#define OFF   0x0045

unsigned char relays[4]={
					RELAY0,
					RELAY1,
					RELAY2,
					RELAY3
					};
					
unsigned char number_patterns[12]={
				  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F, //0
				  SEG_B|SEG_C, //1
				  SEG_A|SEG_B|SEG_D|SEG_E|SEG_G, //2
				  SEG_A|SEG_B|SEG_C|SEG_D|SEG_G, //3
				  SEG_G|SEG_B|SEG_C|SEG_F, //4
				  SEG_A|SEG_G|SEG_C|SEG_D|SEG_F, //5
				  SEG_A|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G, //6
				  SEG_A|SEG_B|SEG_C, //7
				  SEG_A|SEG_B|SEG_C|SEG_D|SEG_E|SEG_F|SEG_G, //8
				  SEG_A|SEG_B|SEG_C|SEG_D|SEG_F|SEG_G, //9
				  SEG_D|SEG_E|SEG_F|SEG_G, // t (for timer)
				  SEG_OFF // OFF
};

#define COMPA_VECT  TIM1_COMPA_vect				// For attiny84

static void 
timer1_init (void)
{
    OCR1A   = (F_CPU / F_INTERRUPTS) - 1;		// compare value: 1/15000 of CPU frequency
    TCCR1B  = (1 << WGM12) | (1 << CS10);		// switch CTC Mode on, set prescaler to 1

    TIMSK1  = (1 << OCIE1A);					// OCIE1A: Interrupt by timer compare
}

ISR(COMPA_VECT)									// Timer1 output compare A interrupt service routine, called every 1/15000 sec
{
  (void) irmp_ISR();							// call irmp ISR
  
  // Here starts the timer for the fan
  if (tim>0) // only it tim is set
  {
	sec++;
		if(sec>15000) // ~ 1 second
		{
			min++;
			if (min>65*tim) // ~ 1 minute
			{ 
			PORTB = 0; // switch off relay
			PORTA = 0xFF; // switch off display
			tim = 0; // time over, set tim to 0;
			min = 0;
			}
	sec = 0;
		}
		
  }
  // End of timer for fan
}



int main (void)
{
    DDRA = 0x07F; // PA0 to PA6 output for LEDs, PA7 iR input
 	PORTA =~ number_patterns[11];	// All LEDs off (common anodes)
		
	DDRB = 0x0F; // Relay port
	PORTB = 0; // Relays off
	
	IRMP_DATA   irmp_data;

    irmp_init();    // initialize IRMP
    timer1_init();  // initialize timer1

    sei ();         // enable interrupts

    while(1)
    {
	 	if (i_tim == 0) // normal fucnction, start fan with different speed, display speed
		{
			
			if (irmp_get_data (&irmp_data))	
			    {
				if((irmp_data.flags & IRMP_FLAG_REPETITION) ==  0) // Debounce
				{ // Get data?
				switch (irmp_data.protocol)
					{ // Checks the protocol of the iR control
					case 0x02:	switch (irmp_data.address)
									{ // Checks the address of the iR control
									case 0xFF00: switch (irmp_data.command)
													{
													case OFF: SEG_PORT =~ SEG_OFF; RELAY_PORT = RELAY0;  // display+relay off
													break;
													case ONE: SEG_PORT =~number_patterns[1]; RELAY_PORT = relays[1]; // speed 1
													break;
													case TWO: SEG_PORT =~number_patterns[2]; RELAY_PORT = relays[2]; // speed 2
													break;
													case THREE: SEG_PORT =~number_patterns[3]; RELAY_PORT = relays[3]; // speed 3
													break;
													case TIMER: SEG_PORT =~number_patterns[10]; RELAY_PORT = relays[0]; i_tim = 1;
													break; // set timer if blue button is pressed (i_tim=1)
													}
									break;
								}
					break;
				}
				}
			}
		}
		
		if (i_tim == 1) // timer is active, now see the parameters (first the speed (speed)), then jump to time setting (i_tim 2 +3)
		{ 
			if (irmp_get_data (&irmp_data))	
				{
				if((irmp_data.flags & IRMP_FLAG_REPETITION) ==  0) // Debounce
				{
				switch (irmp_data.protocol)
					{ // Checks the protocol of the iR control
					case 0x02: switch (irmp_data.address)
									{ // Checks the address of the iR control
									case 0xFF00: switch (irmp_data.command)
													{
													case ONE: SEG_PORT =~number_patterns[1]; speed=1; i_tim = 2; // speed 1
													break;
													case TWO: SEG_PORT =~number_patterns[2]; speed=2; i_tim = 2; // speed 2
													break;
													case THREE: SEG_PORT =~number_patterns[3]; speed=3; i_tim = 2; // speed 3
													break;
													}
									break;
								}
					break;
				}
				}
			}	
		}

		if (i_tim == 2) // time setting, first tenth
		{ 
			if (irmp_get_data (&irmp_data))	
				{
				if((irmp_data.flags & IRMP_FLAG_REPETITION) ==  0) // Debounce
				{
				switch (irmp_data.protocol)
					{ // Checks the protocol of the iR control
					case 0x02: switch (irmp_data.address)
									{ // Checks the address of the iR control
									case 0xFF00: switch (irmp_data.command)
													{
													case ZERO: SEG_PORT =~number_patterns[0]; tim=0; i_tim = 3; 
													break;
													case ONE: SEG_PORT =~number_patterns[1]; tim=10; i_tim = 3; 
													break;
													case TWO: SEG_PORT =~number_patterns[2]; tim=20; i_tim = 3; 
													break;
													case THREE: SEG_PORT =~number_patterns[3]; tim=30; i_tim = 3; 
													break;
													case FOUR: SEG_PORT =~number_patterns[4]; tim=40; i_tim = 3; 
													break;
													case FIVE: SEG_PORT =~number_patterns[5]; tim=50; i_tim = 3; 
													break;
													case SIX: SEG_PORT =~number_patterns[6]; tim=60; i_tim = 3; 
													break;
													case SEVEN: SEG_PORT =~number_patterns[7]; tim=70; i_tim = 3; 
													break;
													case EIGHT: SEG_PORT =~number_patterns[8]; tim=80; i_tim = 3; 
													break;
													case NINE: SEG_PORT =~number_patterns[9]; tim=90; i_tim = 3; 
													break;
													}
									break;
								}
					break;
				}
				}
			}					
		}	

		if (i_tim == 3) // time setting, now unit, add tenth value to tim
		{ 
			if (irmp_get_data (&irmp_data))	
				{
				if((irmp_data.flags & IRMP_FLAG_REPETITION) ==  0) // Debounce
				{
				switch (irmp_data.protocol)
					{ // Checks the protocol of the iR control
					case 0x02: switch (irmp_data.address)
									{ // Checks the address of the iR control
									case 0xFF00: switch (irmp_data.command)
													{
													case ZERO: SEG_PORT =~number_patterns[0]; tim=tim+0; i_tim = 4; 
													break;
													case ONE: SEG_PORT =~number_patterns[1]; tim=tim+1; i_tim = 4; 
													break;
													case TWO: SEG_PORT =~number_patterns[2]; tim=tim+2; i_tim = 4; 
													break;
													case THREE: SEG_PORT =~number_patterns[3]; tim=tim+3; i_tim = 4; 
													break;
													case FOUR: SEG_PORT =~number_patterns[4]; tim=tim+4; i_tim = 4; 
													break;
													case FIVE: SEG_PORT =~number_patterns[5]; tim=tim+5; i_tim = 4;
													break;
													case SIX: SEG_PORT =~number_patterns[6]; tim=tim+6; i_tim = 4; 
													break;
													case SEVEN: SEG_PORT =~number_patterns[7]; tim=tim+7; i_tim = 4; 
													break;
													case EIGHT: SEG_PORT =~number_patterns[8]; tim=tim+8; i_tim = 4; 
													break;
													case NINE: SEG_PORT =~number_patterns[9]; tim=tim+9; i_tim = 4; 
													break;
													}
									break;
								}
					break;
				}
				}
			}					
		}	

		if (i_tim == 4) // again press blue button, switch relay, start timer and reset i_tim to 0
		{ 
			if (irmp_get_data (&irmp_data))	
				{
				if((irmp_data.flags & IRMP_FLAG_REPETITION) ==  0) // Debounce
				{
				switch (irmp_data.protocol)
					{ // Checks the protocol of the iR control
					case 0x02: switch (irmp_data.address)
									{ // Checks the address of the iR control
									case 0xFF00: switch (irmp_data.command)
													{
													case TIMER: SEG_PORT =~number_patterns[10]; RELAY_PORT = relays[speed]; i_tim = 0; 
													break; 
													}
									break;
								}
					break;
				}
				}
			}					
		}			
	
   /* end while */}
	
	
}
