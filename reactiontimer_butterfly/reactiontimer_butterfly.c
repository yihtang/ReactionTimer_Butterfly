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
#define MAX_COUNT ((F_CPU)/ 1024 - 1) // the value that TCNT1 that leads to a 1s delay

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
char score[3];

int main(void)
{	
	// no interrupts
	cli();
	
	LCD_Init();
		
	// use PINB1 and PINB0 as input to check if pressed
	DDRB &= ~((1<< PINB0) | (1 << PINB1));
	// use PINB5 as output (Piezo element on AVR Butterfly) to indicate game start
	DDRB |= (1<<PINB5);
	
	//set up PINB0 to detect if game resets (restarts), all are low initially
	PORTB &= ~((1<< PINB0) | (1 << PINB1));
	// enable external interrupts on PCINT8-PCINT15
	EIMSK |= (1<<PCIE1);
	EIFR |= (1<<PCIF1);
	PCMSK1 |= (1<<PCINT8); // individual interrupt enabler for PCINT8 (PINB0)
	sei();
	
    while(1)
    {
        if (game_start == 0)
		{
			game_buttonpressed = 0;
			game_lastscore = 0;
			
			// disable PINB1 and TNCT CTC interrupt
			TIMSK1 &= ~(1<<OCIE1A);
			PCMSK1 &= ~(1<<PINB1);
		}			
			
		if (game_start == 1)
		{
			game_buttonpressed = 0;
			game_lastscore = 0;
			
			// delay
			_delay_ms(2000);
			
			// set up to give a buzz to indicate game has started
			PORTB |= (1<<PINB5); // on buzz
			
			// enable timer interrupt
			TCNT1 = 0;
			OCR1A = (unsigned int) MAX_COUNT;
			TCCR1A = 0;
			TCCR1B |= (1<<WGM12) | (1<<CS12) | (1<<CS10);	// set CTC mode, prescaler 1024 (Part 1 of 2)
			TCCR1B &= ~((1<<WGM13) | (1<<CS11));			// set CTC mode, prescaler 1024 (Part 2 of 2)
			TIMSK1 |= (1<<OCIE1A);
			
			// enable button interrupt for PCINT8 (PINB1)
			PCMSK1 |= (1<<PINB1);
			
			PORTB &= ~(1<<PINB5); // off buzz
			game_start = 2;
		}
		if ((game_start == 2)&&(game_buttonpressed==1)){
			cli();
			game_lastscore = TCNT1/MAX_COUNT*1000;
			game_buttonpressed = 0; // clear button press to prevent conflict
			//disable interrupts that are set when game_start = 1
			TIMSK1 &= ~(1<<OCIE1A);
			PCMSK1 &= ~(1<<PINB1);	
			sei();
		}
		
		
		itoa(game_lastscore, score, 10);		
		LCD_puts(score);	
		
    }
}

// interrupt service routine for PB0-PB7, corresponding to PCINT8-PCINT15 interrupt enabler
ISR(SIG_PIN_CHANGE1)
{	
	// disable future interrupts to prevent conflicts
	cli();
	LCD_puts("SIG CHANGE");
	
	
	// if PB0 input is high (RESET is pressed)
	if (!(PORTB & (1<<PINB0))){
		game_start = 1;
		LCD_puts("PINB0 HIGH");
	}
	
	// if PB1 is high (user presses button after game started)
	if (!(PORTB & (1<<PINB1))){
		LCD_puts("PINB1 HIGH");
		// disable PINB1 button until game and timer have both started!
		if (game_start == 2)
			game_buttonpressed = 1;
		else
			game_buttonpressed = 0;	
	}
	
	// re-enable interrupts to resume detecting for them
	sei();	
}

ISR(TIMER1_COMPA_vect)
{
	cli();	
	game_start = 2; // remain at state 2, indicating end of game, wait user to press RESET to make game_start = 0
	game_buttonpressed = 0;
	game_lastscore = 999;
	
	//disable interrupts that are set when game_start = 1
	TIMSK1 &= ~(1<<OCIE1A);
	PCMSK1 &= ~(1<<PINB1);
		
	sei();
}