/*
 * reactiontimer_butterfly.c
 *
 * Created: 8/22/2012 12:10:05 PM
 * Author: YihTang Yeo
 * Machine: ASUS-U45JC
 * MCU: AVR Butterfly with ATmega169P
 * Clock Speed: 2MHz by default, can be set up to 8MHz
 */ 

#define F_CPU 2000000
#define MAX_COUNT 5*((F_CPU)/ 1024 - 1) // the value that TCNT1 that leads to a 1s delay

#include <stdlib.h>
#include <avr/io.h>
#include <avr/iom169p.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "LCDdriver.h"
#include "LCDdriver.c"

unsigned char game_start = 0; // 0: game inactive, 1: game started but timer hasn't, 2: game and timer both started, 3: when user presses the button
unsigned int game_lastscore = 0;
char score[8];

int main(void)
{	
	// no interrupts
	cli();
	
	LCD_Init();
		
	// use PIND1 as input
	DDRD &= ~(1<<PIND1);
	// use PINB5 as output (Piezo element on AVR Butterfly) to indicate game start
	DDRB |= (1<<PINB5);
	
	//set up PINB0 to detect if game resets (restarts), all are low initially
	PORTD &= ~(1<<PIND1);
	// enable external interrupts on PCINT8-PCINT15
	EIMSK |= (1<<INT0);
	EICRA &= ~((1<<ISC01) | (1<<ISC00)); // set to detect interrupt request at low level
	sei();
	
    while(1)
    {
        if (game_start == 0)
		{			
			// disable PINB1 and TNCT CTC interrupt
			TIMSK1 &= ~(1<<OCIE1A);
			PORTB &= ~(1<<PINB5); // lights off when game inactive
		}			
			
		else if (game_start == 1)
		{
			game_lastscore = 0;
			
			// delay
			_delay_ms(2000);
			
			// set up to give a buzz to indicate game has started
			PORTB |= (1<<PINB5); // on buzz, keep light on when game is active
			
			// enable timer interrupt
			TCNT1 = 0;
			OCR1A = (unsigned int) MAX_COUNT;
			TCCR1A = 0;
			TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);	// set CTC mode, prescaler 1024 (Part 1 of 2)
			TCCR1B &= ~((1<<WGM13) | (1<<CS11));			// set CTC mode, prescaler 1024 (Part 2 of 2)
			TIMSK1 |= (1<<OCIE1A);
			
			game_start = 2;
		}
		else if (game_start == 3){
			cli();
			game_lastscore = TCNT1/MAX_COUNT*5000;
			game_start = 0; // reset status to inavtive
			//disable interrupts that are set when game_start = 1
			TIMSK1 &= ~(1<<OCIE1A);
			sei();
		}
		
		itoa(game_lastscore, score, 10);		
		LCD_puts(score);	
		
    }
}

// interrupt service routine for PB0-PB7, corresponding to PCINT8-PCINT15 interrupt enabler
ISR(INT0_vect)
{	
	// disable future interrupts to prevent conflicts
	cli();
	
	// if PB0 input is high (RESET is pressed)
	if (PORTB & (1<<PINB0)){
		if (game_start == 0)
			game_start = 1;
		else if (game_start == 1)
			game_start = 2;
		else if (game_start == 2)
			game_start = 3;
		else if (game_start == 3)
			game_start = 0;
	}
	
	// re-enable interrupts to resume detecting for them
	sei();	
}

ISR(TIMER1_COMPA_vect)
{
	cli();	
	game_start = 0; // remain at state 2, indicating end of game, wait user to press RESET to make game_start = 0
	game_lastscore = 4999;
	
	//disable interrupts that are set when game_start = 1
	TIMSK1 &= ~(1<<OCIE1A);
		
	sei();
}