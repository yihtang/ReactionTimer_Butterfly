/*
 * reactiontimer_butterfly.c
 *
 * Created: 8/22/2012 12:10:05 PM
 * Author: YihTang Yeo
 * Machine: ASUS-U45JC
 * MCU: AVR Butterfly with ATmega169P
 * Clock Speed: 2MHz by default, can be set up to 8MHz
 */ 


#include <avr/io.h>
#include <avr/iom169p.h>
#include <avr/interrupt.h>

unsigned char game_start = 0; // 0: game inactive, 1: game started but timer hasn't, 2: game and timer both started
unsigned char game_buttonpressed = 0; // 0: user didn't press the button, or invalid press; 1: user pressed the button

int main(void)
{
	// no interrupts
	cli();
		
	// use PINB1 and PINB0 as input to check if pressed
	DDRB &= ~(1<< PINB0) | (1 << PINB1);
	// use PINB5 as output (Piezo element on AVR Butterfly) to indicate game start
	DDRB |= (1<<PINB5);
	
	//set up PINB0 to detect if game resets (restarts), all are low initially
	PORTB &= ~(1<< PINB0) | (1 << PINB1);
	// enable external interrupts on PCINT8-PCINT15
	EIMSK |= (1<<PCIE1);
	EIFR |= (1<<PCIF1);
	PCMSK1 |= (1<<PCINT8); // individual interrupt enabler for PCINT8
	sei();
	
    while(1)
    {
        //TODO:: Please write your application code 
    }
}

// interrupt service routine for PB0-PB7, corresponding to PCINT8-PCINT15 interrupt enabler
ISR(SIG_PIN_CHANGE1)
{	
	// disable future interrupts to prevent conflicts
	cli();
	
	// if PB0 input is high (RESET is pressed)
	if (PORTB & (1<<PINB0)){
		game_start = 1;
	}
	
	// if PB1 is high (user presses button after game started)
	if (PORTB & (1<<PINB1)){
		// disable PINB1 button until game and timer have both started!
		if (game_start == 2)
			game_buttonpressed = 1;
		else
			game_buttonpressed = 0;	
	}
	
	// re-enable interrupts to resume detecting for them
	sei();	
}