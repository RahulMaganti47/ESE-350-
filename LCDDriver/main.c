#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/signal.h>
#include "uart (1).h"
#include "lcd.h"

#define FREQ 16000000
#define BAUD 9600
#define HIGH 1
#define LOW 0
#define BUFFER 1024
#define BLACK 0x000001
 
//gloabal variables for structs b=ball, player=paddle 1, cpu=paddle 2
ball b;
paddle player;
paddle cpu;

//global variable determining if game is over or ball left screen
int returner; 

//initialize structs
void initializeStruct()  {
	
	//generate a random variable of 0 or 1 to randomize the direction of the ball
	int rando = rand() % 2; 
	
	//initialize ball to center of screen
	b.currX = 32;
	b.currY = 32;
	
	//set velocity depending on random variable value 
	//change the sign of the velocity of the ball according to the parity of rando
	if (rando == 0) {
		b.velX = 1;
		b.velY = 1;
	} else if (rando == 1) {
		b.velX = -1; 
		b.velY = -1; 
	}
	
	//initialize player locations
	player.hi = 36;
	player.lo = 26;
	player.direction = 0;
	 
	//initialize cpu
	cpu.hi = 36;
	cpu.lo = 26;
	cpu.direction = 0;
	
	//initalize gloabal returner to 1 (game ongoing)
	returner = 1;
}

//### initialize the variables ####
int average[10]; 
int sorted[10]; 
int firstime = 0; 
int firstimetotal = 0; 
int averagefirstime = 0;
int aprev = 0; 
int bprev = 0;  

int ballx = 64; 
int bally  = 32; 
int ballxprev = 64; 
int ballyprev = 32; 

// moving average filter to read values from the accelerometer 
int movingaveragefilter() {

	DDRC &= ~(1<<PORTC4); //set pin A4 to input
	DDRC |= (1<<PORTC0) | (1<<PORTC1) | (1<<PORTC2) | (1<<PORTC3); //set the other pins on PORTC to output
	PORTC &= ~(1<<PORTC4); //set pin 4 on port C to read the ADC input
	ADMUX &= 0b11110000;
	ADMUX |= 0b00000100;
	ADCSRA |= (1<<ADSC);
	while(ADCSRA & (1<<ADSC)) {
		
	}
	
	while(firstime < 10) {
		average[firstime] = ADC; 
		firstime++; 
		if (firstime == 10) {
			int j = 0; 
			while(j < 10) {
				firstimetotal += average[j];
				j++; 
			}
		averagefirstime = (firstimetotal / 10);
	}
	}
	
	int newadcval = ADC;
	int i = 0;
	int total = 0;
	int index = 0;
	while(i < 10) {
			total += average[i];
			i++;
	}
	int findaverage = (int) (total / 10); 
	int average1 = 10*findaverage + newadcval - average[index]; 
	average[index] = newadcval;
	index++;
	if (index == 10) index = 0;
	return average1;
}

//function to set the conditions on the ADC to change the direction of movement of the paddles
void changeDirectBalance(int newaverage) {
	if (newaverage > 50) {
		player.direction = 2;
		} else if (newaverage < 50) {
		player.direction = 1;
		} else {
		player.direction = 0;
	}
}

//functionto set the conditions such that the paddle moves according to touch
void changeDirectTouch(int xcoordinate, int ycoordinate) {
	if (xcoordinate <= 63) {
		if (ycoordinate == 0) {
			player.direction = 0;
		}
		else if (ycoordinate <= 32) {
			player.direction = 1;
		}
		else{
			player.direction = 2;
		}
		} else {
		player.direction = 0;
	}
}

//function to update the state and location of the player paddle
void updatep(int newaverage, int xcoordinate, int ycoordinate) {
		
		clearrect(buff, 2, aprev, 4, bprev, 0x00); //instead of clearing entire buffer, clear the rectangle at the previous locations every time
			
		//function 
		//changeDirectTouch(xcoordinate, ycoordinate);
		changeDirectBalance(newaverage);
		
		//
		fillrect(buff, 2, player.lo, 4, player.hi, 0x00); 
		 //save the previous state (location of the bottom and top coordinates of the paddle
		aprev = player.lo;
		bprev = player.hi; 
	
		//move the paddle 
		if (player.direction == 1) {
				player.hi-=2;
				player.lo-=2; 
		} else if (player.direction == 2) {
				player.hi+=2;
				player.lo+=2;
		} else {
		}
		
		//if the cooridnates of paddle go off the screen, ensure that it stays on the screen 
		if (player.hi >= 63) {
			player.hi = 60;
			player.lo = 50;
		}
		if (player.lo <= 1) {
			player.lo = 3;
			player.hi = 13;
		}
}

//function check if there is an impact wiht a paddle. Return the loss variable
int checkPlayerImpact() {

	int loss = 1;

	//if the ball is within the y coordinate bounds of the paddle, set loss to 0 and reflect the ball 
	if ((player.hi >= b.currY) && (player.lo <= b.currY)) {
		loss = 0;
		b.velX = -b.velX;
		
		//set the buzzer to ring every time the ball collides with a paddle
		DDRB |= 0x02; 
		PORTB |= (1<<PORTB1);
		TCCR1B |= (1<<WGM12);
		TCCR1A |= 0x40;
		TCCR1B |= (1<<CS10);
		TCNT1 = 0;
		OCR1A = 18181;
		_delay_ms(200); 
		DDRB &= ~(0x02); 	
		DDRB |= (1<<PORTB2);
		DDRB &= ~(1<<PORTB0);
		DDRD &= ~(1<<PORTD7);

	}
	
	return loss;
}



//function to check the ball's impact with the 
int checkComputerImpact() {

	int loss = 2;
	
	if ((cpu.hi >= b.currY) && (cpu.lo <= b.currY)) {
		loss = 0;
		b.velX = -b.velX; 
	}
		DDRB |= 0x02; 
		PORTB |= (1<<PORTB1);
		TCCR1B |= (1<<WGM12);
		TCCR1A |= 0x40;
		TCCR1B |= (1<<CS10);
		TCNT1 = 0;
		OCR1A = 18181;
		_delay_ms(200); 
		DDRB &= ~(0x02); 
		DDRB |= (1<<PORTB2);
		DDRB &= ~(1<<PORTB0);
		DDRD &= ~(1<<PORTD7);
		
	return loss;
}

//update the position of the ball 
int updateb() {

	ballxprev = b.currX; 
	ballyprev = b.currY; 
	b.currX += b.velX;
	b.currY += b.velY;

	int loss = 0;
	// if the ball exceeds this x-coordinate, check if there is an impact with a paddle
	//if not, reset game, else continue
	if (b.currX >= 124) {
		loss = checkComputerImpact();
		b.currX = 123;
	}
	if (b.currX <= 3) {
		loss = checkPlayerImpact();
		b.currX = 4;
	}
	
	if (b.currY >= 60) {
		b.currY = 60;
		b.velY = -(b.velY);
		
		//sound the buzzer every time the ball collides with a horizontal surface
		DDRB |= 0x02;
		PORTB |= (1<<PORTB1);
		TCCR1B |= (1<<WGM12);
		TCCR1A |= 0x40;
		TCCR1B |= (1<<CS10);
		TCNT1 = 0;
		OCR1A = 18181;
		_delay_ms(200);
		DDRB &= ~(0x02);
		DDRB |= (1<<PORTB2);
		DDRB &= ~(1<<PORTB0);
		DDRD &= ~(1<<PORTD7);
	}
	
	if (b.currY <= 3) {
		b.currY = 3;
		b.velY = -(b.velY);
		//sound the buzzer when the ball collides with the top surface of the game
		DDRB |= 0x02;
		PORTB |= (1<<PORTB1);
		TCCR1B |= (1<<WGM12);
		TCCR1A |= 0x40;
		TCCR1B |= (1<<CS10);
		TCNT1 = 0;
		OCR1A = 18181;
		_delay_ms(200);
		DDRB &= ~(0x02);
		DDRB |= (1<<PORTB2);
		DDRB &= ~(1<<PORTB0);
		DDRD &= ~(1<<PORTD7);
	}
	
	
	//if the ball passes through the coordinates where the score is displayed, do not show the ball
	if (!((b.currX >= 40) && (b.currX <= 80) && (b.currY >= 7) && (b.currY <= 15))) {
		clearcircle(buff, ballxprev, ballyprev, 3, 0x00); 
		fillcircle(buff, b.currX, b.currY, 3, 0);
	}
	
	if (!returner) {
		
		if (b.currX >= 43 && (b.currY >= 24 && b.currY <= 32)) {
			b.velX = -(b.velX); 
			b.velY = -(b.velY); 
		}
	}
	//write_buffer(buff);

	return loss;
	
}

//if the there is a loss on either side, update the score
void updateScore(int loss) {
	//if the loss == 2, update the cpu score
	if (loss == 2){
		cpu.score = cpu.score + 1;
	}
	//if the loss == 1, update the score on the player side
	else if (loss == 1) {
		player.score = player.score + 1;
	}
	
	//draw the score at the top of the screen
	int score1 = 48 + cpu.score;
	int score2 = 48 + player.score;
	drawchar(buff, 43, 1, score1);
	drawchar(buff, 73, 1, score2);
	drawrect(buff, 40, 7, 80, 15, 0);
}

//function to reset the game
void reset() {
	initializeStruct();
	clear_buffer(buff);
	clear_screen();
	fillrect(buff, 2, 26, 4, 36, 0);
	fillrect(buff, 124, 26, 126, 36, 0);
	fillcircle(buff, 63, 31, 3, 0);
	drawrect(buff, 0, 0, 127, 63, 0);
	DDRB |= (1<<PORTB2);
	DDRB |= (1<<PORTB0);
	DDRD |= (1<<PORTD7);
	write_buffer(buff);
	clearcircle(buff, 63, 31, 3, 0);
}

//function to update the CPU
void updateCpu() {
	
	clearrect(buff, 124, cpu.lo, 126, cpu.hi, 0x00); 
	
	if (cpu.lo-3 < b.currY){
		cpu.direction = 2;
	}
	if (cpu.hi+3 > b.currY) {
		cpu.direction = 1;
	}
	
	if (cpu.direction == 1) {
		cpu.hi-=2;
		cpu.lo-=2;
		} else if (cpu.direction == 2) {
		cpu.hi+=2;
		cpu.lo+=2;
		} else {
	}
	
	//keep the cpu paddle within the bounds of the screen
	//put 60 instead of 63 to make the CPU beatable
	if (cpu.hi >= 60) {
		cpu.hi = 60;
		cpu.lo = 50;
	}
	//use 3 instead of 0 to make the CPU beatable
	if (cpu.lo <= 3) {
		cpu.lo = 3;
		cpu.hi = 13;
	}
	fillrect(buff, 124, cpu.lo, 126, cpu.hi, 0x00); //fill the rectangle at the correct position to draw the CPU paddle
}

//function that executes only when the game is over (the score exceeds 9 on either side)
int gameOver() {


	int returner = 1; 
	if ((player.score > 9) || (cpu.score > 9)) {
		DDRB &= ~((1<<PORTB2) | (1<<PORTB0));
		DDRD = (1<<PORTD7) | (1<<PORTB0);
		//clear the display
		clear_screen();
		//clear the values in the buffer
		clear_buffer(buff);
		//draw GAME OVER text in the middle of the screen
		drawstring(buff, 43, 5, "GAME OVER");
		int i = 0;
		//flash the lights on the LCD display to indicate that the game is over
		while(i < 10) {
			returner = 0;
			DDRB ^= (1<<PORTB2);
			_delay_ms(1000);
			DDRB ^= (1<<PORTB0);
			_delay_ms(1000);
			DDRD ^= (1<<PORTD7);
			_delay_ms(1000);
			drawstring(buff, 43, 5, "GAME OVER");
			write_buffer(buff);
			i++;
			DDRB &= ~(0x02); 
		}

	}
	
	int i = 0;
	
	return returner; 
	
}

void updatep2(int xcoordinate, int ycoordinate) {
	//clear the previous rectangle on every iteration of the paddle
	clearrect(buff, 124, cpu.lo, 127, cpu.hi, 0x00); 
	
		//set the direction of the paddle according to the ADC values from the touchscreen
		if (xcoordinate > 63) {
			if (ycoordinate == 0) {
				cpu.direction = 0; 
			}
			else if (ycoordinate <= 32) {
				cpu.direction = 1; 
			} 
			else{
				cpu.direction = 2; 
			}
		} else {
			cpu.direction = 0; 
		} 
		
		//set the movement of the paddle on the screen according to the direction
		if (cpu.direction == 1) {
				cpu.hi-=2;
				cpu.lo-=2; 
		} else if (cpu.direction == 2) {
				cpu.hi+=2;
				cpu.lo+=2;
		} else {
		}
		
		//set the bounds such that the paddles do not go off the screen
		if (cpu.hi >= 63) {
			cpu.hi = 60;
			cpu.lo = 50;
		}
		if (cpu.lo <= 1) {
			cpu.lo = 3;
			cpu.hi = 13;
		}
		//fill a rectangle where the paddle should be
		fillrect(buff, 124, cpu.lo, 126, cpu.hi, 0x00); 
}


int main(void)
{
	uart_init(); 
	//setting up the buzzer
	DDRB |= (0x02);
	PORTB |= (1<<PORTB1);
	//setting up the gpio for backlight
	DDRD |= 0x80;
	PORTD &= ~0x80;
	PORTD |= 0x00;
	
	DDRB |= 0x05;
	PORTB &= ~0x05;
	PORTB |= 0x00;
	
	//lcd initiialisation
	lcd_init();
	lcd_command(CMD_DISPLAY_ON);
	//sendCommands(); 
	lcd_set_brightness(0x18);
	write_buffer(buff);
	_delay_ms(10000);
	clear_buffer(buff);
	double scalingfacx = .130; 
	double scalingfacy = .064;

	player.score = 0;
	cpu.score = 0;
	
	initializeADC(); 
	//sei(); 
	clear_screen();
	initializeStruct();
		
	//initialize the score with the correct ASCII mapping
	int score1 = 48;
	int score2 = 48;
	//initialize the loss variable to see if a player gets a goal
	int loss = 0;
		
	while (1) {
		
		//as long as returner is 1, keep updating the score and resetting game for the next goal
		if (returner) {
			reset();
			updateScore(loss);
		}
		
		//if either score exceeds 9, stop the game and call gameOver
		if ((player.score > 9) || (cpu.score > 9)) {
			returner = gameOver();	
		} 
		
		//if this variable is set to 0, the game is over. Break from this while loop and do not continue allowing user to play
		if (returner == 0) {
			break; 
		}
		
		while (returner)
		{
			_delay_ms(1000); 
			write_buffer(buff);
			int x = readxcoordinate(); //read the x coordinate from ADC
			int y = readycoordinate(); //read the y coordinate from ADC
			
			//Calibrate the x and y coordinates on the touchscreen
			y = 1023-y; 
			if (x == 1023) {
				x = 0; 
			}
			if (y >= 660 && y <=670)  {
				y = 0; 
			}
			x = x - 84;
			y = y - 95; 
			if (x <= 0) {
				x  = 0; 
			}
			if (y <=0) {
				y = 0; 
			}
			
			//multiply the ADC values by the appropriate scaling factors to accurately fit onto the screen
			int xscaled =(int) (x * .135); 
			int yscaled = (int) (y * .0678); 
			//keep the scaled versions of the coordinates within the bounds of the screen
			if (xscaled >=127) {
				xscaled = 127; 
			}
			if (yscaled >= 63) {
				yscaled = 63; 
			}
						 
			int theaverage = movingaveragefilter(average); //call the moving average function on the ADC value
			int newaverage = theaverage % 100;  //keep the ADC values with the 0 - 100 range 
			updatep(newaverage, xscaled, yscaled); //call the player paddle function to update the states of the player  
			loss = updateb(); //call the update ball function 
			updateCpu(); //call the update CPU function
			//if there is a loss (the ball's xcoordinates go off the coorindates of the screen) break from this while loop and re-intialize the game
			if (loss != 0) {
				break;
			}
	}
}
}