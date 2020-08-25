#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gamul.h"
#include <GL/glut.h>
#include <unistd.h>
#include <stdbool.h>

//define constants
#define SCREEN_WIDTH	64
#define SCREEN_HEIGHT	32 

// font set for rendering
const unsigned char fontset[FONTSET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0,		// 0
	0x20, 0x60, 0x20, 0x20, 0x70,		// 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0,		// 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0,		// 3
	0x90, 0x90, 0xF0, 0x10, 0x10,		// 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0,		// 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0,		// 6
	0xF0, 0x10, 0x20, 0x40, 0x40,		// 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0,		// 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0,		// 9
	0xF0, 0x90, 0xF0, 0x90, 0x90,		// A
	0xE0, 0x90, 0xE0, 0x90, 0xE0,		// B
	0xF0, 0x80, 0x80, 0x80, 0xF0,		// C
	0xE0, 0x90, 0x90, 0x90, 0xE0,		// D
	0xF0, 0x80, 0xF0, 0x80, 0xF0,		// E
	0xF0, 0x80, 0xF0, 0x80, 0x80		// F
};

// initialize struct
int initialize(varsGamul_8 *gamer) {
 
	//initialize memory to 0
	memset(gamer->memory, 0, memorysize); 
	//initialize stack pointer to 0
	(gamer->sp) &= 0; 
	//initialize all keys to 0 (not pressed)
	memset(gamer->key, 0, sizeof(gamer->key));
	//initialize registers to 0
	memset(gamer->V, 0, sizeof(gamer->V)); 
	//initialize our stack to 0 (null)
	memset(gamer->stack, 0, sizeof(gamer->stack));  
	//initialize our opcode (instruction) to 0
	(gamer->opcode) = 0; 
	//initialize our program counter to 0x200 start of program memory
	(gamer->pc) = 0x200; 
	//initialize our display to black (0)
	memset(gamer->display, 0, sizeof(gamer->display)); 
	
	//load in our fontset
	int i; 
	for (i = 0; i < 80; i++) {
		 gamer->memory[i] = fontset[i]; 
	}
	
	//load in our file to memory, beginning at 0x200
	int success = load_file(gamer->game, gamer->memory+0x200);

	//return success (0 if success, -1 otherwise)
	return success;
}


void beginEmulationCycle(varsGamul_8 *gamer) {
	//decrement delay if not 0
	if (gamer->delay > 0) {
		(gamer->delay)--; 
	}
	//decrement sound if not 0 and play sound
	if (gamer->sound > 0) {
		system("paplay beep.aiff &> /dev/null &"); 
		(gamer->sound)--; 
	}

	//load next instruction from memory into struct's opcode field
	//memory is defined as a char array so each element is 8 bits; however, opcode is 16 bits
	//so, we load in 2 addresses to get full instruction
	(gamer->opcode) = (gamer->memory[gamer->pc] << 8) | (gamer->memory[gamer->pc+1]);
	//store temporary variables of register X and Y used by some instructions
	unsigned short regX = (gamer->opcode & 0x0F00)>>8; 
	unsigned short regY = (gamer->opcode & 0x00F0)>>4;
	
	switch(gamer->opcode & 0xF000) {
		case 0x0000:
			//Returns from a subroutine. The interpreter sets the program counter to the address at the top of the
			//stack, then subtracts 1 from the stack pointer.
			if ((gamer->opcode == 0x00EE)) {
				gamer->sp--; 
				gamer->pc = gamer->stack[gamer->sp]; 
			//Clears the screen.
			} else if (gamer->opcode == 0x00E0) {
				memset(gamer->display, 0, 4096); 
				gamer->pc += 2;
			}
			break;
		case 0x1000: 
			//Jumps to address NNN.
			gamer->pc = (gamer->opcode & 0x0FFF); 																																																																																																																																																																																																																							
			break;
		case 0x2000:
			//Calls subroutine at NNN
			 gamer->stack[gamer->sp] = gamer->pc+2; 
			 gamer->sp++; 			
			 gamer->pc = (gamer->opcode & 0x0FFF); 
			break; 
		case 0x3000:
			//Skips the next instruction if VX equals NN. (Usually the next instruction is a jump to skip a code block)
			if (gamer->V[regX] == (gamer->opcode & 0x00FF)) {
				gamer->pc = gamer->pc + 4;
			} else {
				gamer->pc += 2;  
			}
			break; 
		case 0x4000: 
			//Skips the next instruction if VX doesn’t equal NN. (Usually the next instruction is a jump to skip a code
			//block)
			if (gamer->V[regX] != (gamer->opcode & 0x00FF)) {
				gamer->pc = gamer->pc + 4;
			} else {
				gamer->pc += 2; 
			}
			break;
		case 0x5000:
			//Skips the next instruction if VX equals VY. (Usually the next instruction is a jump to skip a code block)
			if (gamer->V[regX] == gamer->V[regY]) {
				gamer->pc = gamer->pc + 4; 
			} else {
				gamer->pc += 2;
			}
			break; 	
		case 0x6000: 
			;
			//Sets VX to NN
			gamer->V[regX] = gamer->opcode & 0x00FF;
			gamer->pc += 2;  
			break; 	
		case 0x7000: 
			//Adds NN to VX
			gamer->V[regX] = gamer->V[regX] + (gamer->opcode & 0x00FF);
			gamer->pc += 2; 
			break; 
		case 0x8000: 
			switch(gamer->opcode & 0x000F) {
				case 0x0000: 
					//Sets VX to the value of VY.
					gamer->V[regX] = gamer->V[regY];
					gamer->pc += 2;  
					break; 
				case 0x0001: 
					//Sets VX to VX or VY. (Bitwise OR operation)
					gamer->V[regX] |= gamer->V[regY];
					gamer->pc += 2; 
					break;  
				case 0x0002:  
					//Sets VX to VX and VY. (Bitwise AND operation)
					gamer->V[regX] &= gamer->V[regY]; 
					gamer->pc += 2; 
					break; 
				case 0x0003: 
					//Sets VX to VX xor VY.
					gamer->V[regX] ^= gamer->V[regY]; 
					gamer->pc += 2; 
					break; 
				case 0x0004:
					//Adds VY to VX. VF is set to 1 when there’s a carry, and to 0 when there isn’t.
					gamer->V[regX] += gamer->V[regY];
					if (gamer->V[regY] > (0xFF - gamer->V[regX])) {
						gamer->V[15] = 1;
					}
					else {
						gamer->V[15] = 0;
					}
					gamer->pc += 2; 
					break;
				case 0x0005:
					//VY is subtracted from VX. VF is set to 0 when there’s a borrow, and 1 when there isn’t.
					if(gamer->V[regY] > gamer->V[regX]) {
						gamer->V[15] = 0;
					}
					else {
						gamer->V[15] = 1;
					}
					gamer->V[regX] -= gamer->V[regY];
					 gamer->pc += 2; 
					 break; 
				case 0x0006: 
					//Shifts VX right by one. VF is set to the value of the least significant bit of VX before the shift.
					 gamer->V[15] = gamer->V[regX] & 0x1; 
					 gamer->V[regX] >>= 1; 
					 gamer->pc += 2; 
					 break; 
				case 0x0007:
					;
					//Sets VX to VY minus VX. VF is set to 0 when there’s a borrow, and 1 when there isn’t.
					if(gamer->V[regX] > gamer->V[regY]) {
						gamer->V[15] = 0;
					} 
					else {
						gamer->V[15] = 1;
					}
					gamer->V[regX] = gamer->V[regY] - gamer->V[regX];
					 gamer->pc += 2; 
					 break; 
				case 0x000E: 
					//Shifts VX left by one. VF is set to the value of the most significant bit of VX before the shift.
					gamer->V[15] = gamer->V[regX] >> 7;
					gamer->V[regX] <<= 1; 
					gamer->pc += 2; 
					break; 
				default: 
					//exit if unknown opcode
					printf("unknown opcode\n");
					exit(1); 
			}
			break;
		case 0x9000: 
			//Skips the next instruction if VX doesn’t equal VY. (Usually the next instruction is a jump to skip a code
			//block)
			if (gamer->V[regX] != gamer->V[regY]) {
				gamer->pc = gamer->pc+4; 
			} else {
				gamer->pc += 2; 
			}			
			break; 
		case 0xA000: 
			//Sets I to the address NNN.
			gamer->I = gamer->opcode & 0x0FFF; 
			gamer->pc += 2; 
			break; 
		case 0xB000: 
			//Jumps to the address NNN plus V0.
			gamer->pc = gamer->V[0] + (gamer->opcode & 0x0FFF); 
			gamer->pc += 2; 
			break; 
		case 0xC000: 
			//Sets VX to the result of a bitwise and operation on a random number (Typically: 0 to 255) and NN.
			gamer->V[regX] = (rand() % 256) & (gamer->opcode & 0x00FF); 
			gamer->pc += 2; 
			break; 
		case 0xD000: 
			;
			//Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels. Each row of
			//8 pixels is read as bit-coded starting from memory location I; I value doesn’t change after the execution
			//of this instruction. As described above, VF is set to 1 if any screen pixels are flipped from set to unset
			//when the sprite is drawn, and to 0 if that doesn’t happen. More details in the Display and Graphics
			//section.
			unsigned short height = (gamer->opcode) & 0x000F;
			short x = gamer->V[regX];
			short y = gamer->V[regY]; 
			unsigned short current_pix, newpixel; 
			unsigned int x_new, y_new; 
			int k, w; 
			gamer->V[15] = 0;
			for (k = 0; k < height; k++) {
				y_new = (y + k) % 32; 
				for (w = 0; w < 8; w++) {
					x_new = (x+w) % 64; 
					current_pix = gamer->display[x_new][y_new];
				  	newpixel = (gamer->memory[gamer->I + k] >> (7-w)) & 0x01;
				  	if (newpixel != 0) {
						if (current_pix == 1) {
							gamer->V[15] = 1;
						}
						gamer->display[x_new][y_new] ^= 1; 
				  	} 
				 }
			}
			gamer->pc += 2;
			break; 
		case 0xE000: 	
			switch(gamer->opcode & 0x00FF) {
				case 0x009E:
					//Skips the next instruction if the key stored in VX is pressed. (Usually the next instruction is a jump to
					//skip a code block)
					if (gamer->V[regX] == gamer->keypress) {
						gamer->pc += 4; 
					} else {
						gamer->pc += 2; 
					}
				break;
				case 0x00A1: 
					//Skips the next instruction if the key stored in VX isn’t pressed. (Usually the next instruction is a jump
					//to skip a code block)
					if (gamer->V[regX] != gamer->keypress) {
						gamer->pc += 4; 
					} else {
						gamer->pc += 2; 
					}
				break;
				default:
					//exit if unknown opcode
					printf("unknown opcode"); 
					exit(1); 
			}
		break; 
		case 0xF000:
			switch(gamer->opcode & 0x00FF) {
				case 0x0007: 
					//Sets VX to the value of the delay timer.
					gamer->V[regX] = gamer->delay;
					gamer->pc += 2; 
				break; 
				case 0x000A: 
					;
					//A key press is awaited, and then stored in VX. (Blocking Operation. All instruction halted until next
					//key event)
					gamer->waiting = 1;
					while (gamer->waiting); 
					gamer->V[regX] = gamer->keypress;
					gamer->keypress = 0;
					gamer->pc += 2; 
					break; 
				case 0x0015:
					//Sets the delay timer to VX.
					gamer->delay = gamer->V[regX]; 
					gamer->pc += 2; 
					break; 
				case 0x0018: 
					//Sets the sound timer to VX.
					gamer->sound = gamer->V[regX]; 
					gamer->pc += 2; 
					break; 
				case 0x001E: 
					//Adds VX to I
					if (gamer->I + gamer->V[regX] > 0xFFF) {
						gamer->V[15] = 1;
					}
					else {
						gamer->V[15] = 0;
					}
					gamer->I += gamer->V[regX];
					gamer->pc += 2; 
					break; 
				case 0x0029:
					//Sets I to the location of the sprite for the character in VX. Characters 0-F (in hexadecimal) are represented
					//by a 4x5 font.
					gamer->I = 0x5 * gamer->V[regX]; 
					gamer->pc += 2; 
					break;  
				case 0x0033:
					;
					//Stores the binary-coded decimal representation of VX, with the most significant of three digits at the
					//address in I, the middle digit at I plus 1, and the least significant digit at I plus 2. (In other words, take
					//the decimal representation of VX, place the hundreds digit in memory at location in I, the tens digit at
					//location I+1, and the ones digit at location I+2.)
					unsigned int val  = gamer->V[regX]; 
					gamer->memory[gamer->I] = (val / 100); 
					gamer->memory[gamer->I+1] = (val / 10) % 10;  
					gamer->memory[gamer->I+2] = (val % 100) % 10; 
					gamer->pc += 2; 
					break; 
				case 0x0055: 
					//Stores V0 to VX (including VX) in memory starting at address I.
					for (int j = 0; j <= regX; j++) {
						gamer->memory[gamer->I+j] = gamer->V[j];
					}
					gamer->pc += 2; 
					break; 
				case 0x0065:
					;
					//Fills V0 to VX (including VX) with values from memory starting at address I.
					int a; 
					for (a = 0; a <= regX; a++) {
						gamer->V[a] = gamer->memory[gamer->I+a]; 
					}
					gamer->pc += 2; 
					break; 
				default: 
					//exit if unknown opcode
					printf("unknown opcode");
					exit(1);  
		}
	}
} 




/*	FUNCTION: load_file
 *  -------------------
 *	Loads the given program/game
 *	PARAMETERS: 
 *  file_name: name of file to be loaded
 *  buffer: system memory which will hold program
 *	RETURNS: 0 if successful, -1 if file error
 */
int load_file(char *file_name, unsigned char *buffer)
{
	FILE *file;
	int file_size;

	//return -1 if file name is null
	if (file_name == 0) {
		printf("Error: File name not found->");
		return -1;
	}
	//return -1 if file name does not coorespond to a file in directory
	if (access(file_name, F_OK) == -1) {
		printf("Error: File not found->");
		return -1;
	}
	else {
	// open file stream in binary read-only mode
	file = fopen(file_name, "rb");	//man 3 fopen
	}
	
	fseek(file, 0, SEEK_END);	//man 3 fseek
	file_size = ftell(file);	//man 3 ftell

	//return -1 if file is too large for memory
	if (file_size > 4096) {
		printf("Error: File too large->");
		return -1;
	}

	rewind(file);				//man 3 rewind
	
	fread(buffer, 1, file_size, file);	//man 3 fread
	return 0;

}


/*	FUNCTION: display_func
 *  ----------------------
 *	Sample function that displays a pixel on the screen
 *	PARAMETERS: 
 *  gamer: architecture to be emulated, defined in gamul->h
 *	RETURNS: none
 */
void display_func(varsGamul_8 *gamer)
{
	//unimplemented function used for testing
}