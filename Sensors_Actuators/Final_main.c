/*
 * Lab 3 Sensors and Actuators Part 8 
 *
 * Created: 2/19/2018 8:41:32 PM
 * Author : rahul
 */ 

#include <avr/io.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "uart (1).h"
#include "util/delay.h"

volatile unsigned int group; 
unsigned int timervaluerising;
unsigned int timervaluefalling;
volatile unsigned int pulsewidth;    
volatile int falling = 0; 
volatile int overflows; 

//check the ADC values from the photo-resistor and separates the range into different frequencies or notes
void ranges() {
	TCNT0 = 0; //clear the value of the timer counter just to be safe
	//check ADC val against the states and place appropriate value in OCR0A to generate the necessary frequency or tone
	if (ADC> 45 && ADC < 154) {
		OCR0A = TCNT0 + (int)(8000/64);
		} else if (ADC > 154 && ADC < 263) {
		OCR0A =  TCNT0 +(int) (7130/64);
		} else if (ADC > 263 && ADC < 372) {
		OCR0A =  TCNT0 + (int)(6349/64);
		} else if (ADC > 372 && ADC < 480) {
		OCR0A =  TCNT0 + (int)(5992/64);
		} else if (ADC > 480 && ADC < 590) {
		OCR0A =  TCNT0 + (int)(5340/64);
		} else if (ADC > 590 && ADC < 699) {
		OCR0A =  TCNT0 + (int)(4756/64);
		} else if (ADC > 699 && ADC < 808) {
		OCR0A =  TCNT0 + (int)(4237/64);
		} else if (ADC > 808 && ADC < 917) {
		OCR0A =  TCNT0 + (int)(4000/64);
		} else if (ADC > 917 && ADC < 1024) {
		OCR0A =  TCNT0 + (int)(7130/64);
		} 
}


ISR (TIMER1_OVF_vect) {
	overflows++; 
}

ISR(TIMER1_COMPA_vect) {
	PORTB &= ~(1<<PORTB1);  
	TCCR1B |= (1<<ICES1);  //toggle on interrupt a
	TIMSK1 |= (1<<ICIE1); //initialize input capture interrupt
}

ISR(TIMER1_CAPT_vect) {
	//check if the edge is rising or falling on the Echo pin and act accordingly
	if (TCCR1B & (1<<ICES1)) {
		timervaluerising = ICR1; //capture the timer value for the rising edge
		overflows = 0; 
		TCCR1B &= ~(1<<ICES1); //look for falling edge
		falling = 0; 
	}
	else if (TCCR1B & ~(1<<ICES1))  {
		timervaluefalling = ICR1;  //capture the timer value for the falling edge
		falling = 1; 
		if (timervaluefalling < timervaluerising) {
			overflows--; 
		}
		overflows = 0; 
		TCCR1B |= (1<<ICES1); 
		
	}
 
}

//function to check the pulsewidth difference and divide the range into 8 sets of distances (8 different volumes)
unsigned int rangevoltage(int voltage) {
	if (pulsewidth < 1000) {
		group = -1; 
	}
	else if (pulsewidth > 1000 && pulsewidth < 6175) {
		group = 0; 
	} else if (pulsewidth > 6575 && pulsewidth < 9550) {
		group = 1;
	} else if (pulsewidth > 9950 && pulsewidth < 13000) {
		group = 2;
	} else if (pulsewidth > 13125 && pulsewidth < 16500) {
		group = 3;
	} else if (pulsewidth > 16300 && pulsewidth < 19875) {
		group = 4;
	} else if (pulsewidth > 2100  && pulsewidth < 23250) {
		group = 5;
	} else if (pulsewidth > 23250 && pulsewidth < 26625) {
		group = 6;
	} else if (pulsewidth > 26625 && pulsewidth < 30000) {
		group = 7;		
	} else if (pulsewidth > 30000) {
		group = 8; 
	} else {
		return(voltage); 
	}
	
	return(group); 
}


int main(void)
{
	uart_init(); 
	
	DDRC &= ~(0x01); //set PC0  to analog input 
	
	ADMUX = (1<<REFS0);
	ADMUX &= ~(1<<REFS1);
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
	TCCR0B |= (1<<CS00) | (1<<CS01);  //set prescaler to 64 

	//put the ADC in free running mode 
	ADCSRB &= ~(1<<ADTS0); 
	ADCSRB &= ~(1<<ADTS1); 
	ADCSRB &= ~(1<<ADTS2); 
	ADCSRA |= (1<<ADATE); //set up in auto trigger mode
	ADCSRA |= (1<<ADSC); //begin ADC conversion
	
	//initialize the Timer 0 register
	DDRD = 0x40; 	//set OCR0A PD6 to output
	TCCR0A = (1<<COM0A0); //toggle on compare mode
	TCCR0A |= (1<<WGM01); //set to CTC mode
	
	//initialize the Timer 1 register
	DDRB |= 0x06;
	DDRB &= ~(0x01);
	PORTB |= (1<<PORTB1); //keep line high initially
	TCCR1B |= (1<<CS10);  //start timer with no pre-scaler
	OCR1A = TCNT1 + 160; //set value of OCR1A to generate 10 microsecond pulse
	
	TIMSK1 |= (1<<OCIE1A) | (1<<TOIE1); //enable input capture and overflow interrupts
	sei(); //enable global interrupts
	
	while (1) {
	
		ranges(); //call function to check which state the ADC input is in
		//check the values from the ping sensor only after a falling edge is detected
		if (falling) {
		cli(); //clear all interrupts
		
		//calculate the pulsewidth distance
		pulsewidth = 65535u*overflows + abs(timervaluerising - timervaluefalling);
	
		unsigned int voltage;
		voltage = rangevoltage(voltage);
		//check which group the ping sensor value is in and toggle the necessary bits 
		PORTB &= ~((1<<PORTB2) | (1<<PORTB3) | (1<<PORTB4));
		if (voltage == 0) {
			PORTB &= ~((1<<PORTB2) | (1<<PORTB3) | (1<<PORTB4));
			} else if (voltage == 1) {
			PORTB |= (1<<PORTB2);
			} else if (voltage == 2) {
			PORTB |= (1<<PORTB3);
			} else if (voltage == 3) {
			PORTB |= (1<<PORTB2) | (1<<PORTB3);
			} else if (voltage == 4) {
			PORTB |= (1<<PORTB4);
			} else if (voltage == 5) {
			PORTB |= (1<< PORTB4) | (1<<PORTB2);
			} else if (voltage == 6) {
			PORTB |= (1<<PORTB4) | (1<<PORTB3);
			} else if (voltage == 7 || voltage == 8) {
			PORTB |= (1<<PORTB4) | (1<<PORTB3) | (1<<PORTB2);
		}
		
		falling = 0; //set to check for rising edge on ECHO pin
		//calculate the distance
		float pulsewidthf = (float)pulsewidth; 
		float dist = (pulsewidthf / 16000000) * 17000; 
		unsigned int disti; 
		disti = (int)dist; 
		
		//print the distance values in centimeters
		if (disti < 1) {
			
		} else {
			printf("%u cm \n", disti); 
		}*/
		
		//print the pulsewidth 
		if (pulsewidth < 1000) {
			
		} else {
			printf("%u\n", pulsewidth); 
		}

		sei(); //enable global interrupts
		}

	
	}
}