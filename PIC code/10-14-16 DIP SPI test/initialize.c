/*******************************************************
Name: initialize.c
Date: 5/9/2016
Authors: Mark Price
Comments: Initialization functions for timers and peripherals
*******************************************************/
#define FCY 40000000UL

#include "p33FJ64MC202.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h" 		//Include Defitions and function prototypes
#include <string.h>
#include <stdlib.h>
#include "libpic30.h"		//FCY must be defined before libpic30.h is included

/*
 * These defines set the UART Baud Rate
 * Make sure that the Baud Rate in this program is equal to the rate set on the XBee/serial interface/whatever
 */
//#define BAUDRATE 57600
//#define BRGVAL ((FCY/BAUDRATE)/16) - 1

unsigned int TxBufferA[3] __attribute__((space(dma)));
//unsigned int TxBufferB[3] __attribute__((space(dma)));
unsigned int RxBufferA[3] __attribute__((space(dma)));
//unsigned int RxBufferB[3] __attribute__((space(dma)));
unsigned int dataIn[3];
unsigned int dataOut[3];
//unsigned int ADCBufferA[MAX_NUM_ADC+1][SAMP_BUFF_SIZE] __attribute__((space(dma),aligned(128)));
//unsigned int ADCBufferB[MAX_NUM_ADC+1][SAMP_BUFF_SIZE] __attribute__((space(dma),aligned(128)));

extern long int run_time;	// run time in milliseconds
extern int wait_flag;		// signals end of 1 millisecond sample period

extern float samp_time;

//Flags to tell if SPI can be sent on module 1 or module 2
extern int spi1_flag;

//////////////////////////////////////////////////////////
// Initialization of Clock Frequency

//Configuration Bits for Clock Initialization Parameters
_FOSCSEL(FNOSC_FRC);				// Select Internal FRC at POR
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF);	// Enable Clock Switching and Output Clock to OSC2
_FWDT(FWDTEN_OFF); 					// Turn off Watchdog Timer
_FICD(JTAGEN_OFF & ICS_PGD1);		// Turn off JTAG and communicate on PGC1/EMUC1 and PGD1/EMUD1

void init_clock(void)
{
    // Configure PLL prescaler, PLL postscaler, PLL divisor
    // 7.37 MHz * 43 / 2 / 2 = 79.227 MHz
    PLLFBD = 41;			// M = 43
    CLKDIVbits.PLLPOST=0;	// N2 = 2
    CLKDIVbits.PLLPRE=0;	// N1 = 2

    // Initiate Clock Switch to Internal FRC with PLL (NOSC = 0b001)
    __builtin_write_OSCCONH(0x01);
    __builtin_write_OSCCONL(0x01);

    // Wait for Clock switch to occur
    while (OSCCONbits.COSC != 0b001);

    // Wait for PLL to lock
    while(OSCCONbits.LOCK!=1) {};
} //end init_clock

//////////////////////////////////////////////////////////
// Timer1 interupt service routine
// Used to enforce Sample Time
void _ISR _T1Interrupt(void)
{
	IFS0bits.T1IF = 0;
	wait_flag = 1; //Signal end of sample time	
	run_time++;
} // end T1Interupt


//////////////////////////////////////////////////////////
// Intialize Sample Time (equal to value of float samp_time)
void init_samptime(void)
{
	TMR1 = 0;
//	T1CON = 0x8030;		// TMR1 on, prescalar set to 1:8 Tclk/2
    T1CONbits.TCKPS = 0b11; // Prescale set to 1:256 Tclk/2
    PR1 = (samp_time*155) - 1; 	// Set the period register (samp_time in milliseconds)
	_T1IF = 0; 			// Clear Flag
	_T1IE = 1;			// Enable Source
    T1CONbits.TON = 1;
} //end init_samptime


//////////////////////////////////////////////////////////
//Initialize Pins
void init_pins(void)
/* Pin Overview Map
//		RA0 - FREE              (PIN2) 
//		RA1 - FREE       		(PIN3) 
//		RA2 - FREE              (PIN9)
//		RA3 - FREE              (PIN10)
//		RA4 - FREE              (PIN12)
//		RB0	- PGED1				(PIN4) 
//		RB1	- PGEC1				(PIN5) 
//		RB2 - FREE              (PIN6) 
//		RB3 - FREE              (PIN7)      
//		RB4 - FREE              (PIN11) 
//		RB5 - FREE              (PIN14)
//		RB6 - FREE              (PIN15) 
//		RB7 - FREE              (PIN16)
//		RB8 - FREE              (PIN17)
//		RB9 - FREE              (PIN18)
//		RB10 - SDO1 (SPI1)      (PIN21)
//		RB11 - FREE             (PIN22)
//		RB12 - SCK1 (SPI1)      (PIN23)	input
//		RB13 - SDI1 (SPI1)      (PIN24) input
//		RB14 - FREE             (PIN25)
//		RB15 - FREE             (PIN26)     
*/ 

{
	AD1PCFGL = 0xffff;       // All analog pins set to digital

//    TRISA = 0x0000;     // analog set to digital out
	TRISA = 0x0000; 	// all output
	ODCA = 0x0000;			// no open drain
	LATA = 0x0000;		// all pins set to zero

//    TRISB = 0x3040; 	// 	all analog set to digital out
	TRISB = 0x0000; 	// 	RB12,13 input,rest PORTB pins set to output
	ODCB = 0x0000;		//	no open drain
	LATB = 0x0000;		//	all pins set to zero

} // end init_pins


//////////////////////////////////////////////////////////////////////////////
// SPI Initialization
//////////////////////////////////////////////////////////////////////////////

/*
 * Initialize SPI1 module for slave mode (Rasp. Pi is master)
 */
void init_spi1(void)
{
    //SPI1 pins - Slave mode (RasPi master)
    RPOR5bits.RP10R = 7;       // Assign SDO1 to RP10, Pin 21 - OUTPUT
//    RPOR6bits.RP12R = 8;
    RPINR20bits.SCK1R = 12;     // Assign SCK1 to RP12, Pin 23 - INPUT
    RPINR20bits.SDI1R = 13;     // Assign SDI1 to RP13, Pin 24 - INPUT 
   
    
    SPI1STAT = 0x0;  // disable the SPI module (just in case)
    // Interrupt Controller Settings
    SPI1BUF = 0;
    IFS0bits.SPI1IF = 0; // Clear the Interrupt flag
    IEC0bits.SPI1IE = 0; // Disable the interrupt
    // SPI1CON1 Register Settings
    SPI1CON1 = 0;
    SPI1CON1bits.DISSCK = 0; // Internal Serial Clock is enabled
    SPI1CON1bits.DISSDO = 0; // SDOx pin is controlled by the module
    SPI1CON1bits.MODE16 = 0; // Communication is byte-wide (8 bits)
    SPI1CON1bits.SMP = 0; // Input data is sampled at the middle of data
    // output time.
    SPI1CON1bits.CKE = 0; // Serial output data changes on transition
    // from Idle clock state to active clock state
    SPI1CON1bits.CKP = 0; // Idle state for clock is a low level; active
    // state is a high level
    SPI1CON1bits.MSTEN = 0; // Master mode disabled
    ////To get 1Mhz = 64Mhz/2/2/16
        //need to be adjusted to account for the fact I am using a different clock frequency than the original code, maybe adjust UART equivilant instead
	SPI1CON1bits.SPRE= 0b110;//secondary prescale is 2:1
	SPI1CON1bits.PPRE= 0b01;//primary prescale is 16:1
    SPI1STATbits.SPIROV=0; // No Receive Overflow has occurred
    SPI1STATbits.SPIEN = 1; // Enable SPI module

    IFS0bits.SPI1IF = 0; // Clear the Interrupt flag
    IEC0bits.SPI1IE = 1; // Enable the interrupt
    IPC2bits.SPI1IP = 6; // Set interrupt priority
    // Force First word after Enabling SPI
//    DMA0REQbits.FORCE=1;
//    while(DMA0REQbits.FORCE==1);


}

///* 
// * Initializes Direct Memory Access
// * This allows the send and receive operations with the Playsurface to happen in the background 
// * while the CPU executes the main control loop.
// */
//void init_dma0_spi1_tx(void)
//{
//    // Set up DMA Channel 0 for transmit in continuous ping-pong mode:
//	DMA0CONbits.AMODE = 0;
//	DMA0CONbits.MODE  = 0;
//	DMA0CONbits.DIR   = 1;
//	DMA0CONbits.SIZE  = 0; // Continuous, Post-Inc., RAM-to-peripheral
//	DMACS0 = 0;
//	DMA0CNT = 2; // 3 DMA requests 
//	DMA0REQ = 0x000A; // Select Transfer done SPI1
//	DMA0PAD = (volatile unsigned int) &SPI1BUF;
//	DMA0STA= __builtin_dmaoffset(TxBufferA);
////	DMA0STB= __builtin_dmaoffset(TxBufferB);
//	
//	
//	IFS0bits.DMA0IF  = 0;			// Clear DMA interrupt
//	IEC0bits.DMA0IE  = 1;			// Enable DMA interrupt
//	DMA0CONbits.CHEN = 1;			// Enable DMA Channel	
//}
//
//void init_dma1_spi1_rx(void)
//{
//	// Set up DMA Channel 1 for receive in continuous ping-pong mode:
//	DMA1CONbits.AMODE = 0;
//	DMA1CONbits.MODE  = 0;
//	DMA1CONbits.DIR   = 0;
//	DMA1CONbits.SIZE  = 0; // One-shot, Post-Inc., RAM-to-peripheral
////    DMA1CON = 0x0002; // continuous, post-inc, Periph-RAM
////	DMA1CONbits.SIZE = 1;
//	DMA1CNT = 2; // 3 DMA requests
//	DMA1REQ = 0x000A; // Select Transfer done SPI1
//	DMA1PAD = (volatile unsigned int) &SPI1BUF;
//	DMA1STA = __builtin_dmaoffset(RxBufferA);
////	DMA1STB = __builtin_dmaoffset(RxBufferB);
//	_DMA1IF = 0; // Clear DMA interrupt
//	_DMA1IE = 1; // Enable DMA interrupt
//	DMA1CONbits.CHEN = 1; // Enable DMA Channel
//}
    
//void init_spi1_buff(void)
//{
//unsigned int i;
//    for(i=0;i<3;i++)
//        TxBufferA[i]=i;
//    for(i=0;i<3;i++)
//        TxBufferB[i]=16+i;        
//        
//   	for(i=0;i<3;i++){
//        RxBufferA[i]=0xDEED;
//        RxBufferB[i]=0xDEED;
//		}	
//}

/*
 * Sets the SPI interupt (communication completed)
 */
void _ISR _SPI1Interrupt(void)
{
	spi1_flag++;
	IFS0bits.SPI1IF = 0;
//	receiveSPImsg();
	SPI1STATbits.SPIROV = 0;
}

//unsigned int TxBufferCount = 0; // Keep record of the buffer
//unsigned int RxBufferCount = 0;
//
//void _ISR _DMA0Interrupt(void)
//{
////	static unsigned int BufferCount = 0; // Keep record of the buffer that
//	// contains TX data
////	if(TxBufferCount == 0)
////	{
//        TxBufferA[0] = 0x01;
//        TxBufferA[1] = 0x00;
//        TxBufferA[2] = 0x02;
////		TxData(TxBufferA); // Transmit SPI data in
//		// DMA RAM Primary buffer
////	}
////	else
////	{
////        TxBufferB[0] = 0x01;
////        TxBufferB[1] = 0x00;
////        TxBufferB[2] = 0x02;        
////		TxData(TxBufferB); // Transmit SPI data in DMA RAM
//		// Secondary buffer
////	}
////	TxBufferCount ^= 1;
//    
//    //while(SPI1STATbits.SPITBF);
//    
//    //DMA0CONbits.CHEN = 1;
//	IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt flag
//}
//
//void _ISR _DMA1Interrupt(void)
//{
////    static unsigned int BufferCount = 0; // Keep record of the buffer
//	// that contains RX data
////	if(RxBufferCount == 0)
////	{
//		RxData(RxBufferA); // Process received SPI data in
//		// DMA RAM Primary buffer
////	}
////	else
////	{
////		RxData(RxBufferB); // Process received SPI data in
//		// DMA RAM Secondary buffer
////	}
////	RxBufferCount ^= 1;
//	IFS0bits.DMA1IF = 0;
////    SPI_wait_flag = 1;
//}

//void TxData(unsigned int *TxBuff){
//
//	int n;
//    
//    //spi1_flag = 0;
//    int inputSize = sizeof(TxBufferA)/sizeof(TxBufferA[0]);
//
//    
//    for (n = 0; n < inputSize; n++){
//        TxBuff[n] = dataOut[n];
//    }
//    
//    TxBuff[0] = 0x01;
//    TxBuff[1] = 0x00;
//    TxBuff[2] = 0x02;
////    TxBuff[2] = 0x33;
//    
//}
//
//void RxData(unsigned int *RxBuff){
//
//	int n;
//    int inputSize = sizeof(RxBufferA)/sizeof(RxBufferA[0]);
//    
//    for (n = 0; n < inputSize; n++){
//        dataIn[n] = RxBuff[n];
//        dataOut[n] = dataIn[n];
//    }
//
//}

/////////////////////////////////////////////////////////////
// Basic utility functions
/////////////////////////////////////////////////////////////

/*
 * Return the sign of a float (-1, 1, or 0)
 */
int fsign(float num)
{
	//if(num == 0) return 0;
	int sign = (0 < num) - (num < 0);
	delay();
	return sign;
}

/*
 * Empty method to give a slight delay when updating registers
 */
void delay (void){
int i;
for (i=0; i<15; i++){};
}