/*
 * Kyle Miho kmiho001@ucr.edu
 * Lab Section: 023
 * Assignment: Final Project
 *
 * I acknowledge all content contained herein, excluding template or example code, is my own original work.
  */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include "io.c"
#include <stdio.h>

// set bit 'k' to 'b' with 'x'
unsigned char SetBit(unsigned char x,unsigned char k, unsigned char b ){
	return (b ? x | (0x01 << k) : x & ~(0x01 << k));
}
// get bit 'k' from 'x'
unsigned char GetBit(unsigned char x, unsigned char k) {
	return ((x & (0x01 <<k)) != 0);
}

//NOTE*** THIS NEW CODE TARGETS PB6 NOT PB3

void set_PWM(double frequency) {
	
	
	// Keeps track of the currently set frequency
	// Will only update the registers when the frequency
	// changes, plays music uninterrupted.
	static double current_frequency;
	if (frequency != current_frequency) {

		if (!frequency) TCCR3B &= 0x08; //stops timer/counter
		else TCCR3B |= 0x03; // resumes/continues timer/counter
		
		// prevents OCR3A from overflowing, using prescaler 64
		// 0.954 is smallest frequency that will not result in overflow
		if (frequency < 0.954) OCR3A = 0xFFFF;
		
		// prevents OCR3A from underflowing, using prescaler 64					// 31250 is largest frequency that will not result in underflow
		else if (frequency > 31250) OCR3A = 0x0000;
		
		// set OCR3A based on desired frequency
		else OCR3A = (short)(8000000 / (128 * frequency)) - 1;

		TCNT3 = 0; // resets counter
		current_frequency = frequency;
	}
}

void PWM_on() {
	TCCR3A = (1 << COM3A0);
	// COM3A0: Toggle PB6 on compare match between counter and OCR3A
	TCCR3B = (1 << WGM32) | (1 << CS31) | (1 << CS30);
	// WGM32: When counter (TCNT3) matches OCR3A, reset counter
	// CS31 & CS30: Set a prescaler of 64
	set_PWM(0);
}

void PWM_off() {
	TCCR3A = 0x00;
	TCCR3B = 0x00;
}

//--------Find GCD function -------------------------------
unsigned long int findGCD (unsigned long int a,
unsigned long int b)
{
	unsigned long int c;
	while(1){
		c = a%b;
		if(c==0){return b;}
		a = b;
		b = c;
	}
	return 0;
}
//--------End find GCD function ---------------------------
//--------Task scheduler data structure--------------------
// Struct for Tasks represent a running process in our
// simple real-time operating system.
/*Tasks should have members that include: state, period, a
measurement of elapsed time, and a function pointer.*/
typedef struct _task {
	//Task's current state, period, and the time elapsed
	// since the last tick
	signed char state;
	unsigned long int period;
	unsigned long int elapsedTime;
	//Task tick function
	int (*TickFct)(int);
} task;
//--------End Task scheduler data structure----------------
//--------Shared Variables---------------------------------
unsigned char SM2_output = 0x00;
unsigned char SM3_output = 0x00;
unsigned char pause = 0;
//--------End Shared Variables-----------------------------//--------User defined FSMs-------------------------------


void selectline_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}

void red_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = 0x08;
		// set SER = next bit of data to be sent.
		PORTD |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTD |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTD = 0x00;
}

void blue_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTD = 0x80;
		// set SER = next bit of data to be sent.
		PORTD |= (((data >> i) & 0x01) << 4);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x20;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTD |= 0x40;
	// clears all lines in preparation of a new transmission
	PORTD = 0x00;
}

//--------User defined FSMs--------------------------------
enum SM1_States 
{ IDLE,
	LEFT_PRESS,
	RIGHT_PRESS} SM1_States;
// Monitors button connected to PA0. When the button is
// pressed, shared variable "pause" is toggled.

unsigned char x1 = 0;//character position x from 0 -> 7
unsigned char y1 = 1;//character position x from 0 -> 7
unsigned char x2 = 6;
unsigned char y2 = 7;
unsigned char left = 0;
unsigned char right = 0;
int boardData[8][8] =
{
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2},
{2,2,2,2,2,2,2,2}
};
// 0 = Player 2
// 1 = Player 1
// 2 = Empty
// 3 = Wall
// 4 = Bomb
// 5 = Explosion
//(0,0) starts at bottom right corner, +1 to x = 1 left, +1 to y = 1 up

void initializeBoard()
{
	for (unsigned char i = 0; i < 8; ++i)
	{
		for (unsigned char j = 0; j < 8; ++j)
		{
			boardData[i][j] = 2;
		}
	}
	//walls initialize
	for (unsigned char i = 0; i < 8; ++i)
	{
		boardData[i][0] = 3;
	}
	for (unsigned char i = 1; i < 8; ++i)
	{
		boardData[7][i] = 3;
	}
		boardData[1][2] = 3;
		boardData[1][4] = 3;
		boardData[1][6] = 3;
		boardData[3][2] = 3;
		boardData[3][4] = 3;
		boardData[3][6] = 3;
		boardData[5][2] = 3;
		boardData[5][4] = 3;
		boardData[5][6] = 3;
	
	//add player
		boardData[0][1] = 1;
		boardData[6][7] = 0;
}
int SMTick1(int state) //character movement x
{
	// Local Variables
	//State machine transitions
	unsigned char left = ~PINA & 0x04;
	unsigned char right = ~PINA & 0x08;
	
	switch(state)
	{
		case IDLE:
		 if (((left && !right) && (x1 > 0)) && (boardData[x1 -1][y1] == 2))
		 {
			 //fix player coordinates then adjust state
			 if (boardData[x1][y1] != 4) //if bomb present, dont change to empty tile
			 {
				boardData[x1][y1] = 2;
			 }
			 boardData[x1-1][y1] = 1; 
			 --x1;
			 state = LEFT_PRESS;
		 } else if (((right && !left) && (x1 < 7)) && (boardData[x1 +1][y1] == 2))
		 {
			if (boardData[x1][y1] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x1][y1] = 2;
			}
			 boardData[x1+1][y1] = 1;
			 ++x1;
			 state = RIGHT_PRESS;
		 } else
		 {
			 state = IDLE;
		 }
		 break;
		 
		 case LEFT_PRESS:
		 if (!left)
		 {
			 state = IDLE;
		 } else
		 {
			 state = LEFT_PRESS;
		 }
		 break;
		 
		 case RIGHT_PRESS:
		 if (!right)
		 {
			 state = IDLE;
		 } else
		 {
			 state = RIGHT_PRESS;
		 }
		 break;
		 
		 default:
			state = IDLE;
	}
	return state;
}
enum SM6_States
{ IDLE6,
	LEFT_PRESS6,
RIGHT_PRESS6} SM6_States;

int SMTick6(int state) //character movement x player 2
{
	// Local Variables
	//State machine transitions
	unsigned char left = ~PINA & 0x40;
	unsigned char right = ~PINA & 0x80;
	
	switch(state)
	{
		case IDLE6:
		if (((left && !right) && (x2 > 0)) && (boardData[x2 -1][y2] == 2))
		{
			//fix player coordinates then adjust state
			if (boardData[x2][y2] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x2][y2] = 2;
			}
			boardData[x2-1][y2] = 0;
			--x2;
			state = LEFT_PRESS6;
		} else if (((right && !left) && (x2 < 7)) && (boardData[x2 +1][y2] == 2))
		{
			if (boardData[x2][y2] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x2][y2] = 2;
			}
			boardData[x2+1][y2] = 0;
			++x2;
			state = RIGHT_PRESS6;
		} else
		{
			state = IDLE6;
		}
		break;
		
		case LEFT_PRESS6:
		if (!left)
		{
			state = IDLE6;
		} else
		{
			state = LEFT_PRESS6;
		}
		break;
		
		case RIGHT_PRESS6:
		if (!right)
		{
			state = IDLE6;
		} else
		{
			state = RIGHT_PRESS6;
		}
		break;
		
		default:
		state = IDLE6;
	}
	return state;
}

//--------User defined FSMs--------------------------------
enum SM2_States
{ IDLE2,
	UP_PRESS,
	DOWN_PRESS} SM2_States;
// Monitors button connected to PA0. When the button is
// pressed, shared variable "pause" is toggled.

enum SM7_States
{ IDLE7,
	UP_PRESS7,
DOWN_PRESS7} SM7_States;
// Monitors button connected to PA0. When the button is
// pressed, shared variable "pause" is toggled.

int SMTick2(int state) //character movement x
{
	// Local Variables
	//State machine transitions
	unsigned char up = ~PINA & 0x01;
	unsigned char down = ~PINA & 0x02;
	
	switch(state)
	{
		case IDLE2:
		if (((up && !down) && (y1 > 0)) && (boardData[x1][y1-1] == 2))
		{
			//switch empty space with person1
			if (boardData[x1][y1] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x1][y1] = 2;
			}
			boardData[x1][y1-1] = 1;
			--y1;
			state = UP_PRESS;
		}
		if (((down && !up) && (y1 < 7)) && (boardData[x1][y1+1] == 2))
		{
			if (boardData[x1][y1] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x1][y1] = 2;
			}
			boardData[x1][y1+1] = 1;
			++y1;
			state = DOWN_PRESS;
		}
		break;
		
		case UP_PRESS:
		if (!up)
		{
			state = IDLE2;
		}
		break;
		
		case DOWN_PRESS:
		if (!down)
		{
			state = IDLE2;
		}
		break;
		
		default:
		state = IDLE2;
	}
		
	
	return state;
}


int SMTick7(int state) //character movement x
{
	// Local Variables
	//State machine transitions
	unsigned char up = ~PINA & 0x10;
	unsigned char down = ~PINA & 0x20;
	
	switch(state)
	{
		case IDLE7:
		if (((up && !down) && (y2 > 0)) && (boardData[x2][y2-1] == 2))
		{
			//switch empty space with person1
			if (boardData[x2][y2] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x2][y2] = 2;
			}
			boardData[x2][y2-1] = 0;
			--y2;
			state = UP_PRESS7;
		}
		if (((down && !up) && (y2 < 7)) && (boardData[x2][y2+1] == 2))
		{
			if (boardData[x2][y2] != 4) //if bomb present, dont change to empty tile
			{
				boardData[x2][y2] = 2;
			}
			boardData[x2][y2+1] = 0;
			++y2;
			state = DOWN_PRESS7;
		}
		break;
		
		case UP_PRESS7:
		if (!up)
		{
			state = IDLE7;
		}
		break;
		
		case DOWN_PRESS7:
		if (!down)
		{
			state = IDLE7;
		}
		break;
		
		default:
		state = IDLE7;
	}
	
	
	return state;
}


enum SM3_States
{ IDLE3,
	BLUE_PRINT,
	RED_PRINT,
	RED_PRINT2
} SM3_States;
// Monitors button connected to PA0. When the button is
// pressed, shared variable "pause" is toggled.

char blueData[] = {0xFE,0xAA,0xFE,0xAA,0xFE,0xAA,0xFE,0x00};
unsigned char blueRow = 0;
unsigned char redRow = 0;

int SMTick3(int state) //character movement x
{
	switch(state)
	{
		case IDLE3:
		state = BLUE_PRINT;
		
		break;
		
		case BLUE_PRINT:
		red_data(0xFF);
		blue_data(0xFF);
		selectline_data(1 << blueRow);
		blue_data(blueData[blueRow]);
		++blueRow;
		if (blueRow > 8)
		{
			state = RED_PRINT;
			blueRow = 0;
			blue_data(0xFF);
		}
		
		break;
		
		case RED_PRINT:
		red_data(0xFF);
		blue_data(0xFF);
		selectline_data(1 << x1);
		red_data(~(1 << y1));
		++redRow;
		if (redRow > 2)
		{
			state = RED_PRINT2;
			redRow = 0;
			red_data(0xFF);
		}
		break;
		
				
		case RED_PRINT2:
		red_data(0xFF);
		blue_data(0xFF);
		selectline_data(1 << x2);
		red_data(~(1 << y2));
		++redRow;
		if (redRow > 2)
		{
			state = BLUE_PRINT;
			redRow = 0;
			red_data(0xFF);
		}
		break;
		
		default:
		state = IDLE3;
	}


	return state;
}

enum SM4_States //bomb for state
{ IDLE4,
	PLAYER1_A
} SM4_States;

enum SM8_States
{
	IDLE8,
	PLAYER2_A
} SM8_States;

unsigned char player2lose = 0;
unsigned char player1lose = 1;

int bombCoordinates[3][2] = 
{
{-1,-1},
{-1,-1},
{-1,-1}
};
unsigned char bombMax = 3;
int bombTimer[3] = 
{ 0, 0, 0};
int explosionTimer[3] =
{ 0, 0, 0};
int explosionON[3] =
{ 0, 0, 0};
unsigned char lastBomb = -1; //index of last bomb placed
int bombCoordinates2[3][2] =
{
	{-1,-1},
	{-1,-1},
	{-1,-1}
};
unsigned char bombMax2 = 3;
int bombTimer2[3] =
{ 0, 0, 0};
int explosionTimer2[3] =
{ 0, 0, 0};
int explosionON2[3] =
{ 0, 0, 0};
unsigned char lastBomb2 = -1; //index of last bomb placed

void explodeBombPlayer1(unsigned char bombNum)
{
		red_data(0xFF);
		blue_data(0xFF);
		selectline_data(1 << bombCoordinates[bombNum][0]);
		red_data(~(1 << bombCoordinates[bombNum][1]));
		char ix = bombCoordinates[bombNum][0];
		char iy = bombCoordinates[bombNum][1];

		if ((ix == x1) && (iy == y1))
		{
			player1lose = 1;
		} else if ((ix == x2) && (iy == y2))
		{
			player2lose = 1;
		}
		//right explosion
		char right = 1;
		while ((ix >= 0) && (ix <= 7) && (right))
		{
			if (boardData[ix][iy] != 3)
			{
				//set to explosion
				//boardData[ix-1][iy] = 5
				selectline_data(1 << ix);
				red_data(~(1 << iy));
				
				if (boardData[ix][iy] == 4) //if you come in contact with another bomb
				{
					//search for bomb
					for(unsigned i = 0; i < bombMax; ++i)
					{
						if((ix == bombCoordinates[i][0]) && (iy == bombCoordinates[i][1])) //explosion touches bomb
						{
							bombTimer[i] = 0;
							explosionON[i] = 1;
							explosionTimer[i] = 0;
							boardData[(bombCoordinates[i][0])][(bombCoordinates[i][1])] = 5;
						}
					}
				} else if (boardData[ix][iy] == 1)
				{
					player1lose = 1;
					//player 1 dies
				} else if (boardData[ix][iy] == 0)
				{
					player2lose = 1;
				}
			} else
			{
				right = 0;
			}
			--ix;
		}
		//left explosion
        ix = bombCoordinates[bombNum][0];
		char left = 1;
		while ((ix >= 0) && (ix <= 7) && (left))
		{
			if (boardData[ix][iy] != 3)
			{
				//set to explosion
				//boardData[ix-1][iy] = 5
				selectline_data(1 << ix);
				red_data(~(1 << iy));
				if (boardData[ix][iy] == 4) //if you come in contact with another bomb
				{
					//search for bomb
					for(unsigned i = 0; i < bombMax; ++i)
					{
						if((ix == bombCoordinates[i][0]) && (iy == bombCoordinates[i][1])) //explosion touches bomb
						{
							bombTimer[i] = 0;
							explosionON[i] = 1;
							explosionTimer[i] = 0;
							boardData[(bombCoordinates[i][0])][(bombCoordinates[i][1])] = 5;
						}
					}
				}
				else if (boardData[ix][iy] == 1)
				{
					 player1lose = 1;
					 //player 1 dies
				} else if (boardData[ix][iy] == 0)
				{
					player2lose = 1;
				}
				} else
				{
					left = 0;
				}
				++ix;
		}
		
		//up explosion
		ix = bombCoordinates[bombNum][0];
		iy = bombCoordinates[bombNum][1];
		char up =1;
		while ((iy >= 0) && (iy <= 7) && (up))
		{
			if (boardData[ix][iy] != 3)
			{
				//set to explosion
				//boardData[ix-1][iy] = 5
				selectline_data(1 << ix);
				red_data(~(1 << iy));
				if (boardData[ix][iy] == 4) //if you come in contact with another bomb
				{
					//search for bomb
					for(unsigned i = 0; i < bombMax; ++i)
					{
						if((ix == bombCoordinates[i][0]) && (iy == bombCoordinates[i][1])) //explosion touches bomb
						{
							bombTimer[i] = 0;
							explosionON[i] = 1;
							explosionTimer[i] = 0;
							boardData[(bombCoordinates[i][0])][(bombCoordinates[i][1])] = 5;
						}
					}
				} else if (boardData[ix][iy] == 1)
				{
					player1lose = 1;
					//player 1 dies
				} else if (boardData[ix][iy] == 0)
				{
					player2lose = 1;
				}
				} else
				{
					up = 0;
				}
				++iy;
		}

		//down explosion
		ix = bombCoordinates[bombNum][0];
		iy = bombCoordinates[bombNum][1];
		char down =1;
		while ((iy >= 0) && (iy <= 7) && (down))
		{
			if (boardData[ix][iy] != 3)
			{
				//set to explosion
				//boardData[ix-1][iy] = 5
				selectline_data(1 << ix);
				red_data(~(1 << iy));
				if (boardData[ix][iy] == 4) //if you come in contact with another bomb
				{
					//search for bomb
					for(unsigned i = 0; i < bombMax; ++i)
					{
						if((ix == bombCoordinates[i][0]) && (iy == bombCoordinates[i][1])) //explosion touches bomb
						{
							bombTimer[i] = 0;
							explosionON[i] = 1;
							explosionTimer[i] = 0;
							boardData[(bombCoordinates[i][0])][(bombCoordinates[i][1])] = 5;
						}
					}
				}
				else if (boardData[ix][iy] == 1)
				{
					 player1lose = 1;
					 //player 1 dies
				}
				 else if (boardData[ix][iy] == 0)
				 {
					 player2lose = 1;
				 }
			} else
			{
				down = 0;
			}
			--iy;
		}		
		iy = bombCoordinates[bombNum][1];
		red_data(~(1 << iy));
		red_data(0xFF);
		blue_data(0xFF);
		explosionTimer[bombNum] = explosionTimer[bombNum] + 1;
		if (explosionTimer[bombNum] > 30) //explosion ends
		{
			explosionTimer[bombNum] = 0;
			explosionON[bombNum] = 0;
			boardData[(bombCoordinates[bombNum][0])][(bombCoordinates[bombNum][1])] = 2; //set explosion area to empty
			bombCoordinates[bombNum][0] = -1;
			bombCoordinates[bombNum][1] = -1;
		}
}

void explodeBombPlayer2(unsigned char bombNum)
{
	red_data(0xFF);
	blue_data(0xFF);
	selectline_data(1 << bombCoordinates2[bombNum][0]);
	red_data(~(1 << bombCoordinates2[bombNum][1]));
	char ix = bombCoordinates2[bombNum][0];
	char iy = bombCoordinates2[bombNum][1];
		if (boardData[ix][iy] == 1)
		{
			player1lose = 1;
		} else if (boardData[ix][iy] == 0)
		{
			player2lose = 1;
		}
	//right explosion
	char right = 1;
	while ((ix >= 0) && (ix <= 7) && (right))
	{
		if (boardData[ix][iy] != 3)
		{
			//set to explosion
			//boardData[ix-1][iy] = 5
			selectline_data(1 << ix);
			red_data(~(1 << iy));
			
			if (boardData[ix][iy] == 4) //if you come in contact with another bomb
			{
				//search for bomb
				for(unsigned i = 0; i < bombMax2; ++i)
				{
					if((ix == bombCoordinates2[i][0]) && (iy == bombCoordinates2[i][1])) //explosion touches bomb
					{
						bombTimer2[i] = 0;
						explosionON2[i] = 1;
						explosionTimer2[i] = 0;
						boardData[(bombCoordinates2[i][0])][(bombCoordinates2[i][1])] = 5;
					}
				}
			} else if (boardData[ix][iy] == 1)
			{
				player1lose = 1;
				//player 1 dies
			} else if (boardData[ix][iy] == 0)
			{
				player2lose = 1;
			}
		} else
		{
			right = 0;
		}
		--ix;
	}
	//left explosion
	ix = bombCoordinates2[bombNum][0];
	char left = 1;
	while ((ix >= 0) && (ix <= 7) && (left))
	{
		if (boardData[ix][iy] != 3)
		{
			//set to explosion
			//boardData[ix-1][iy] = 5
			selectline_data(1 << ix);
			red_data(~(1 << iy));
			if (boardData[ix][iy] == 4) //if you come in contact with another bomb
			{
				//search for bomb
				for(unsigned i = 0; i < bombMax2; ++i)
				{
					if((ix == bombCoordinates2[i][0]) && (iy == bombCoordinates2[i][1])) //explosion touches bomb
					{
						bombTimer2[i] = 0;
						explosionON2[i] = 1;
						explosionTimer2[i] = 0;
						boardData[(bombCoordinates2[i][0])][(bombCoordinates2[i][1])] = 5;
					}
				}
			}
			else if (boardData[ix][iy] == 1)
			{
				player1lose = 1;
				//player 1 dies
			} else if (boardData[ix][iy] == 0)
			{
				player2lose = 1;
			}
		} else
		{
			left = 0;
		}
		++ix;
	}
	
	//up explosion
	ix = bombCoordinates2[bombNum][0];
	iy = bombCoordinates2[bombNum][1];
	char up =1;
	while ((iy >= 0) && (iy <= 7) && (up))
	{
		if (boardData[ix][iy] != 3)
		{
			//set to explosion
			//boardData[ix-1][iy] = 5
			selectline_data(1 << ix);
			red_data(~(1 << iy));
			if (boardData[ix][iy] == 4) //if you come in contact with another bomb
			{
				//search for bomb
				for(unsigned i = 0; i < bombMax2; ++i)
				{
					if((ix == bombCoordinates2[i][0]) && (iy == bombCoordinates2[i][1])) //explosion touches bomb
					{
						bombTimer2[i] = 0;
						explosionON2[i] = 1;
						explosionTimer2[i] = 0;
						boardData[(bombCoordinates2[i][0])][(bombCoordinates2[i][1])] = 5;
					}
				}
			} else if (boardData[ix][iy] == 1)
			{
				player1lose = 1;
				//player 1 dies
			} else if (boardData[ix][iy] == 0)
			{
				player2lose = 1;
			}
		} else
		{
			up = 0;
		}
		++iy;
	}

	//down explosion
	ix = bombCoordinates2[bombNum][0];
	iy = bombCoordinates2[bombNum][1];
	char down =1;
	while ((iy >= 0) && (iy <= 7) && (down))
	{
		if (boardData[ix][iy] != 3)
		{
			//set to explosion
			//boardData[ix-1][iy] = 5
			selectline_data(1 << ix);
			red_data(~(1 << iy));
			if (boardData[ix][iy] == 4) //if you come in contact with another bomb
			{
				//search for bomb
				for(unsigned i = 0; i < bombMax2; ++i)
				{
					if((ix == bombCoordinates2[i][0]) && (iy == bombCoordinates2[i][1])) //explosion touches bomb
					{
						bombTimer2[i] = 0;
						explosionON2[i] = 1;
						explosionTimer2[i] = 0;
						boardData[(bombCoordinates2[i][0])][(bombCoordinates2[i][1])] = 5;
					}
				}
			}
			else if (boardData[ix][iy] == 1)
			{
				player1lose = 1;
				//player 1 dies
			}
			else if (boardData[ix][iy] == 0)
			{
				player2lose = 1;
			}
		} else
		{
			down = 0;
		}
		--iy;
	}
	iy = bombCoordinates2[bombNum][1];
	red_data(~(1 << iy));
	red_data(0xFF);
	blue_data(0xFF);
	explosionTimer2[bombNum] = explosionTimer2[bombNum] + 1;
	if (explosionTimer2[bombNum] > 30) //explosion ends
	{
		explosionTimer2[bombNum] = 0;
		explosionON2[bombNum] = 0;
		boardData[(bombCoordinates2[bombNum][0])][(bombCoordinates2[bombNum][1])] = 2; //set explosion area to empty
		bombCoordinates2[bombNum][0] = -1;
		bombCoordinates2[bombNum][1] = -1;
	}
}

int SMTick4(int state) 
{
	unsigned char aButton = ~PINC & 0x01;
	for (unsigned char i = 0; i < bombMax; ++i)
	{
		//if a bomb exists, count down its timer for explosion
		if((explosionON[i] == 0) && (bombCoordinates[i][0] > -1) && (bombCoordinates[i][1] > -1))
		{
			bombTimer[i] = bombTimer[i] + 1;
		}
		if (explosionON[i])
		{
			explodeBombPlayer1(i);
		}
		if (bombTimer[i] > 60)
		{
		//remove bomb & reset timer & blow up area
			bombTimer[i] = 0;
			explosionON[i] = 1;
			explosionTimer[i] = 0;
			boardData[(bombCoordinates[i][0])][(bombCoordinates[i][1])] = 5;
		}
	}
		unsigned char nextBomb = 0; //index of next bomb to be placed
		if (lastBomb+1 == bombMax)
		{
			nextBomb = 0;
		} else
		{
			nextBomb = lastBomb + 1;
		}
	switch(state)
	{
		case IDLE4:
		if (aButton && (bombCoordinates[nextBomb][0] == -1)&& (bombCoordinates[nextBomb][1] == -1))
		{
			//remove old bomb
			//if ((player1bomb1x >= 0) && (player1bomb1y >= 0))
			//{
				//boardData[player1bomb1x][player1bomb1y] = 2;
			//}
			boardData[x1][y1] = 4;
			//add new bomb
			bombCoordinates[nextBomb][0] = x1;
			bombCoordinates[nextBomb][1] = y1;
			bombTimer[nextBomb] = 0;
			lastBomb = nextBomb; //next bomb in cycle
			state = PLAYER1_A;
		}
		break;
		
		case PLAYER1_A:
		if (!aButton)
		{
			state = IDLE4;
		}
		break;
		
		default:
		state = IDLE4;
	}

	return state;
}



int SMTick8(int state)
{
	unsigned char aButton = ~PINC & 0x02;
	for (unsigned char i = 0; i < bombMax2; ++i)
	{
		//if a bomb exists, count down its timer for explosion
		if((explosionON2[i] == 0) && (bombCoordinates2[i][0] > -1) && (bombCoordinates2[i][1] > -1))
		{
			bombTimer2[i] = bombTimer2[i] + 1;
		}
		if (explosionON2[i])
		{
			explodeBombPlayer2(i);
		}
		if (bombTimer2[i] > 60)
		{
			//remove bomb & reset timer & blow up area
			bombTimer2[i] = 0;
			explosionON2[i] = 1;
			explosionTimer2[i] = 0;
			boardData[(bombCoordinates2[i][0])][(bombCoordinates2[i][1])] = 5;
		}
	}
	unsigned char nextBomb = 0; //index of next bomb to be placed
	if (lastBomb2+1 == bombMax2)
	{
		nextBomb = 0;
	} else
	{
		nextBomb = lastBomb2 + 1;
	}
	switch(state)
	{
		case IDLE8:
		if (aButton && (bombCoordinates2[nextBomb][0] == -1)&& (bombCoordinates2[nextBomb][1] == -1))
		{
			boardData[x2][y2] = 4;
			//add new bomb
			bombCoordinates2[nextBomb][0] = x2;
			bombCoordinates2[nextBomb][1] = y2;
			bombTimer2[nextBomb] = 0;
			lastBomb2 = nextBomb; //next bomb in cycle
			state = PLAYER2_A;
		}
		break;
		
		case PLAYER2_A:
		if (!aButton)
		{
			state = IDLE8;
		}
		break;
		
		default:
		state = IDLE8;
	}

	return state;
}

enum SM5_States //bomb for state
{ 
	IDLE5,
	RED_BOMB
} SM5_States;
int player1Blink[3] = 
{0,0,0};

int SMTick5(int state) //bomb displays
{
	switch(state)
	{
		case IDLE5:
		state = RED_BOMB;
		break;
		
		case RED_BOMB:
		red_data(0xFF);
		blue_data(0xFF);
		for (unsigned i = 0; i < bombMax; ++i)
		{
			if (((bombCoordinates[i][0] >= 0) && (bombCoordinates[i][1] >= 0)) 
			&& (player1Blink[i] < 5) && !explosionON[i]) //if the bomb exists but did not blow up yet
			{
				selectline_data(1 << (bombCoordinates[i][0]));
				red_data(~(1 << (bombCoordinates[i][1])));
			    	
			}
			player1Blink[i] = player1Blink[i] + 1;
			if (player1Blink[i] > 20)
			{
				player1Blink[i] = 0;
			}
			red_data(0xFF);
			blue_data(0xFF);
		}
		for (unsigned i = 0; i < bombMax; ++i)
		{
			if (((bombCoordinates2[i][0] >= 0) && (bombCoordinates2[i][1] >= 0))
			&& (player1Blink[i] < 10) && !explosionON2[i]) //if the bomb exists but did not blow up yet
			{
				selectline_data(1 << (bombCoordinates2[i][0]));
				red_data(~(1 << (bombCoordinates2[i][1])));
				
			}
			player1Blink[i] = player1Blink[i] + 1;
			if (player1Blink[i] > 80)
			{
				player1Blink[i] = 0;
			}
			red_data(0xFF);
			blue_data(0xFF);
		}
		break;
		
		default:
			state = IDLE5;
	}
	return state;
}

// TimerISR() sets this to 1. C programmer should clear to 0.
volatile unsigned char TimerFlag = 0;
// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1 ms.
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks
void TimerOn() {
	// AVR timer/counter controller register TCCR1
	// bit3 = 0: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: pre-scaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s
	TCCR1B = 0x0B;
	// AVR output compare register OCR1A.
	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	OCR1A = 125;// AVR timer interrupt mask register
	// bit1: OCIE1A -- enables compare match interrupt
	TIMSK1 = 0x02;
	//Initialize avr counter
	TCNT1=0;
	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;
	//Enable global interrupts: 0x80: 1000000
	SREG |= 0x80;
}
void TimerOff() {
	// bit3bit1bit0=000: timer off
	TCCR1B = 0x00;
}
void TimerISR() {
	TimerFlag = 1;
}
// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect) {
	// CPU automatically calls when TCNT1 == OCR1
	// (every 1 ms per TimerOn settings)
	// Count down to 0 rather than up to TOP (results in a more efficient comparison)
	_avr_timer_cntcurr--;
	if (_avr_timer_cntcurr == 0) {
		// Call the ISR that the user uses
		TimerISR();
		_avr_timer_cntcurr = _avr_timer_M;
	}
}
// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void resetGame()
{

initializeBoard();
bombCoordinates[0][0] = -1;
bombCoordinates[1][0] = -1;
bombCoordinates[2][0] = -1;
bombCoordinates[0][1] = -1;
bombCoordinates[1][1] = -1;
bombCoordinates[2][1] = -1;
bombTimer[0] = 0;
bombTimer[1] = 0;
bombTimer[2] = 0;
explosionTimer[0] = 0;
explosionTimer[1] = 0;
explosionTimer[2] = 0;
explosionON[0] = 0;
explosionON[1] = 0;
explosionON[2] = 0;
bombCoordinates2[0][0] = -1;
bombCoordinates2[1][0] = -1;
bombCoordinates2[2][0] = -1;
bombCoordinates2[0][1] = -1;
bombCoordinates2[1][1] = -1;
bombCoordinates2[2][1] = -1;
bombTimer2[0] = 0;
bombTimer2[1] = 0;
bombTimer2[2] = 0;
explosionTimer2[0] = 0;
explosionTimer2[1] = 0;
explosionTimer2[2] = 0;
explosionON2[0] = 0;
explosionON2[1] = 0;
explosionON2[2] = 0;
lastBomb = -1; //index of last bomb placed
lastBomb2 = -1; //index of last bomb placed
x1 = 0;
y1 = 1;
x2 = 6;
y2 = 7;
player1Blink[0] = 0;
player1Blink[1] = 0;
player1Blink[2] = 0;
selectline_data(0xFF);
while	(!(~PINC & 0x04))
{
	if (player1lose)
	{
		red_data(0x00);
		blue_data(0xFF);
	} else if (player2lose)
	{
		red_data(0xFF);
		blue_data(0x00);
	}
}
player1lose = 0;
player2lose = 0;
}



int main(void)
{
	// PORTB set to output, outputs init 0s
	DDRA = 0x00, PORTA = 0xFF;
	DDRB = 0xFF; PORTB = 0x00;
	// PC7..4 outputs init 0s, PC3..0 inputs init 1s
	DDRC = 0xF0; PORTC = 0x0F;
	DDRD = 0xFF, PORTD = 0x00;
	initializeBoard();
	
	
	// Period for the tasks
	unsigned long int SMTick1_calc = 30;
	unsigned long int SMTick2_calc = 30;
	unsigned long int SMTick3_calc = 1;
	unsigned long int SMTick4_calc = 30;
	unsigned long int SMTick5_calc = 5;
	unsigned long int SMTick6_calc = 30;
	unsigned long int SMTick7_calc = 30;
	unsigned long int SMTick8_calc = 30;


	//Calculating GCD
	unsigned long int tmpGCD = 1;
	tmpGCD = findGCD(SMTick1_calc, SMTick3_calc);
	
	//Greatest common divisor for all tasks
	// or smallest time unit for tasks.
	unsigned long int GCD = tmpGCD;

	//Recalculate GCD periods for scheduler
	unsigned long int SMTick1_period = SMTick1_calc/GCD;
	unsigned long int SMTick2_period = SMTick2_calc/GCD;
	unsigned long int SMTick3_period = SMTick3_calc/GCD;
	unsigned long int SMTick4_period = SMTick4_calc/GCD;
	unsigned long int SMTick5_period = SMTick5_calc/GCD;
	unsigned long int SMTick6_period = SMTick6_calc/GCD;
	unsigned long int SMTick7_period = SMTick7_calc/GCD;
	unsigned long int SMTick8_period = SMTick8_calc/GCD;
	
	//Declare an array of tasks
	static task task1, task2, task3, task4, task5, task6, task7, task8;
	task *tasks[] = { &task1, &task2, &task3, &task4, &task5
		, &task6, &task7, &task8};
	const unsigned short numTasks =
	sizeof(tasks)/sizeof(task*);
	
	// Task 1
	task1.state = -1;
	task1.period = SMTick1_period;
	task1.elapsedTime = SMTick1_period;
	task1.TickFct = &SMTick1;	
	// Task 2
	task2.state = -1;
	task2.period = SMTick2_period;
	task2.elapsedTime = SMTick2_period;
	task2.TickFct = &SMTick2;
	// Task 3
	task3.state = -1;		
	task3.period = SMTick3_period;
	task3.elapsedTime = SMTick3_period;
	task3.TickFct = &SMTick3;
	// Task 4
	task4.state = -1;
	task4.period = SMTick4_period;
	task4.elapsedTime = SMTick4_period;
	task4.TickFct = &SMTick4;
	// Task 5
	task5.state = -1;
	task5.period = SMTick5_period;
	task5.elapsedTime = SMTick5_period;
	task5.TickFct = &SMTick5;
	// Task 6
	task6.state = -1;
	task6.period = SMTick6_period;
	task6.elapsedTime = SMTick6_period;
	task6.TickFct = &SMTick6;
	// Task 7
	task7.state = -1;
	task7.period = SMTick7_period;
	task7.elapsedTime = SMTick7_period;
	task7.TickFct = &SMTick7;
	// Task 8
	task8.state = -1;
	task8.period = SMTick8_period;
	task8.elapsedTime = SMTick8_period;
	task8.TickFct = &SMTick8;
	// Set the timer and turn it on
	TimerSet(GCD);
	TimerOn();
	
	// Scheduler for-loop iterator
	unsigned short i;
	while(1) {
		if (player1lose || player2lose)
		{
			resetGame();
		}
		// Scheduler code
		for ( i = 0; i < numTasks; i++ ) {
			// Task is ready to tick
			if ( tasks[i]->elapsedTime ==
			tasks[i]->period ) {
				// Setting next state for task
				tasks[i]->state =
				tasks[i]->TickFct(tasks[i]->state);
				// Reset elapsed time for next tick.
				tasks[i]->elapsedTime = 0;
			}
			tasks[i]->elapsedTime += 1;
		}
		while(!TimerFlag);
		TimerFlag = 0;
	}
	// Error: Program should not exit!
	return 0;
}