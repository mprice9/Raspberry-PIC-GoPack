/******************************************************************
Name: main.c
Author: Mark Price
Date: 5/9/2016

INCLUDE FILES: p33FJ64MC204.h, p33FJ64MC204.gld (Linker Script),
functions.c, support.h
******************************************************************/
#define FCY 40000000UL

#include "p33FJ64MC202.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h" 		//Include Defitions and function prototypes
#include <stdio.h>
#include <stdlib.h>
#include "libpic30.h"		//FCY must be defined before libpic30.h is included

////////////////////////////////////////////////////////////////////////////////////////////
// Global variable declarations
////////////////////////////////////////////////////////////////////////////////////////////

// Initialization/runtime variables
long int run_time = 0;			// 1 msec increments, resets at 2^16 msec
float period;
int wait_flag = 0;				// signal for end of sample time
float samp_time = 1; 			// milliseconds
float p;


// SPI communication variables
int spi1_flag;
extern unsigned int dataIn[3];
extern unsigned int dataOut[3];

/////////////////////////////////////////////////////////////////
// Start Main Function
/////////////////////////////////////////////////////////////////

int main()
{

//Initialization Routines
init_clock();			// initialize Clock 80 MHz/40 MIPS
//init_samptime();		// initialize sample time
init_pins();			// initialize pins

//Set up SPI (slave - RasPi, master - peripherals)
//init_spi1_buff();

//init_dma0_spi1_tx();
//init_dma1_spi1_rx();
init_spi1();            // slave
_RB9 = 1;
int n = 0;

dataOut[0] = 0x01;
dataOut[1] = 0x00;
dataOut[2] = 0x02;
SPI1BUF = 0x01;
//Local variables in main

while(1)
{

    while(!spi1_flag);
    dataIn[n] = SPI1BUF;
    spi1_flag = 0;
    
    n++;
    if(n > 2) n=0;
    if(n==0) _RA2 = 1;
    else _RA2 = 0;

    SPI1BUF = dataOut[n];
    //while(spi1_flag<1);		//use Spi flag to indicate buffer is full
    

    
}
return 0;
} // end main

