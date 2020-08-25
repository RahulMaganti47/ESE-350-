#ifndef GAMUL_H
#define GAMUL_H

#define memorysize 4096
#define displaysize 64*32
#define SCREEN_WIDTH	64
#define SCREEN_HEIGHT	32
#define FONTSET_SIZE	80

typedef struct varsGamul_8 {
	 
	//stores current instruction
	unsigned short opcode; 
	//stores value of display
	unsigned short display[SCREEN_WIDTH][SCREEN_HEIGHT]; 
	//stores memory
	unsigned char memory[memorysize]; 
	//stores stack pointer
	unsigned short sp; 
	//stores registers
	unsigned char V[16];
	//stores program counter
	unsigned int pc; 
	//stores delay timer
	unsigned char delay;
	//stores sound timer
	unsigned char sound;
	//stores index register I
	unsigned int I; 
	//stores value of keys (1 = pressed; 0 = not pressed)
	unsigned char key[16]; 
	//stores stack
	unsigned int stack[0x10];
	//stores current key pressed
	unsigned short keypress;
	//boolean determining if we are currently waiting for a keypress
	unsigned short waiting; 
	
	//string for filename
	char *game; 
	
} varsGamul_8;

//function declarations
int initialize();
void beginEmulationCycle(varsGamul_8 *gamer);
int load_file(char *file_name, unsigned char *buffer);
extern void display_func(varsGamul_8 *gamer);

#endif