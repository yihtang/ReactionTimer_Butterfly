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
#define MAX_COUNT 9765 // for 5 seconds 

#include <stdlib.h>
#include <avr/io.h>
#include <avr/iom169p.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "LCDdriver.h"
#include "LCDdriver.c"

unsigned char game_start = 0; // 0: game inactive, 1: game started but timer hasn't, 2: game and timer both started
unsigned char game_buttonpressed = 0; // 0: user didn't press the button, or invalid press; 1: user pressed the button
unsigned int game_lastscore = 0;
char score[8];

void checkPORTB();

int main(void)
{	
	// no interrupts
	cli();
	
	LCD_Init();
		
	// use PINB1 and PINB2 as input to check if pressed
	DDRB &= ~((1<< PINB1) | (1 << PINB2));
	// use PINB5 as output (Piezo element on AVR Butterfly) to indicate game start
	DDRB |= (1<<PINB5);
	
	//set up PINB0 to detect if game resets (restarts), all are high (5V) initially
	PORTB &= ~(1<<PINB5);
	PORTB |= 0xFF;
	// enable external interrupts on PCINT8-PCINT15
	EIMSK |= (1<<PCIE1);
	EIFR |= (1<<PCIF1);
	PCMSK1 |= (1<<PCINT9) | (1<<PCINT10); // individual interrupt enabler for PCINT9 (PINB1) and PCINT10 (PINB2)
	sei();
	
    while(1)
    {
		
        if (game_start == 0)
		{
			game_buttonpressed = 0;
			game_lastscore = 0;
			PORTB &= ~(1<<PINB5); // turn off light when game inactive
			
			// disable PINB1 and TNCT CTC interrupt
			TIMSK1 &= ~(1<<OCIE1A);
		}			
			
		else if (game_start == 1)
		{
			game_buttonpressed = 0;
			game_lastscore = 0;
			
			// delay
			_delay_ms(10000);
			
			// set up to give a buzz to indicate game has started
			PORTB |= (1<<PINB5); // on buzz, turn on light when game is active
			
			// enable timer interrupt
			TCNT1 = 0;
			OCR1A = (unsigned int) MAX_COUNT;
			TCCR1A = 0;
			TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);	// set CTC mode, prescaler 1024 (Part 1 of 2)
			TCCR1B &= ~((1<<WGM13) | (1<<CS11));			// set CTC mode, prescaler 1024 (Part 2 of 2)
			TIMSK1 |= (1<<OCIE1A);
			
			game_start = 2;
		}
		else if ((game_start == 2)&&(game_buttonpressed==1)){
			cli();
			game_lastscore = TCNT1/(MAX_COUNT/5000);
			game_buttonpressed = 0; // clear button press to prevent conflict
			//disable interrupts that are set when game_start = 1
			TIMSK1 &= ~(1<<OCIE1A);
			PCMSK1 &= ~(1<<PCINT10);	
			PORTB &= ~(1<<PINB5); // turn off LED when done
			sei();
		}
		
		
		itoa(PORTB, score, 16);		
		LCD_puts(score);	
		
    }
}

// interrupt service routine for PB0-PB7, corresponding to PCINT8-PCINT15 interrupt enabler
ISR(PCINT1_vect)
{	
	
	unsigned char PORTBINFO = PORTB;
	
	// if PB0 input is low (RESET is pressed)
	if ((PORTBINFO & (1<<PINB1))){
		game_start = 1;
		//LCD_puts("PINB0 LOW");
	}
	
	// if PB1 is low (user presses button after game started)
	else if ((PORTBINFO & (1<<PINB2))){
		//LCD_puts("PINB1 LOW");
		// disable PINB1 button until game and timer have both started!
		if (game_start == 2)
			game_buttonpressed = 1;
		else
			game_buttonpressed = 0;	
	}
}

ISR(TIMER1_COMPA_vect)
{
	game_start = 2; // remain at state 2, indicating end of game, wait user to press RESET to make game_start = 0
	game_buttonpressed = 0;
	game_lastscore = 999;
	PORTB &= ~(1<<PINB5); // turn off light when it's done
	
	//disable interrupts that are set when game_start = 1
	TIMSK1 &= ~(1<<OCIE1A);
	PCMSK1 &= ~(1<<PCINT10);
		
}