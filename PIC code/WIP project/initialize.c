/*******************************************************
Name: initialize.c
Date: 5/9/2016
Authors: Mark Price
Comments: Initialization functions for timers and peripherals
*******************************************************/
#define FCY 40000000UL

#include "p33FJ64MC204.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h" 		//Include Defitions and function prototypes
#include <string.h>
#include <stdlib.h>
#include "libpic30.h"		//FCY must be defined before libpic30.h is included

/*
 * These defines set the UART Baud Rate
 * Make sure that the Baud Rate in this program is equal to the rate set on the XBee/serial interface/whatever
 */
#define BAUDRATE 57600
#define BRGVAL ((FCY/BAUDRATE)/16) - 1

unsigned int TxBufferA[RASPI_BUF_SIZE] __attribute__((space(dma)));
unsigned int TxBufferB[RASPI_BUF_SIZE] __attribute__((space(dma)));
unsigned int RxBufferA[RASPI_BUF_SIZE] __attribute__((space(dma)));
unsigned int RxBufferB[RASPI_BUF_SIZE] __attribute__((space(dma)));
unsigned int ADCBufferA[MAX_NUM_ADC][SAMP_BUFF_SIZE] __attribute__((space(dma),aligned(64)));
unsigned int ADCBufferB[MAX_NUM_ADC][SAMP_BUFF_SIZE] __attribute__((space(dma),aligned(64)));

extern long int run_time;	// run time in milliseconds
extern int wait_flag;		// signals end of 1 millisecond sample period

extern float samp_time;
extern float p;

extern int pwm1_duty;
extern int pwm2_duty;

extern int SPI_wait_flag;

extern long int enc1pos;

unsigned int aInBuff[MAX_NUM_ADC][SAMP_BUFF_SIZE];
//unsigned int ain0Buff[SAMP_BUFF_SIZE];
//unsigned int ain1Buff[SAMP_BUFF_SIZE];
//unsigned int ain2Buff[SAMP_BUFF_SIZE];
//unsigned int ain3Buff[SAMP_BUFF_SIZE];
//unsigned int ain4Buff[SAMP_BUFF_SIZE];
//unsigned int ain5Buff[SAMP_BUFF_SIZE];
//unsigned int ain6Buff[SAMP_BUFF_SIZE];
//unsigned int ain7Buff[SAMP_BUFF_SIZE];
//unsigned int ain8Buff[SAMP_BUFF_SIZE];
int sampleCounter;

//Flags to tell if SPI can be sent on module 1 or module 2
extern int spi1_flag;
extern int spi2_flag;

//////////////////////////////////////////////////////////
// Initialization of Clock Frequency

//Configuration Bits for Clock Initialization Parameters
_FOSCSEL(FNOSC_FRC);				// Select Internal FRC at POR
_FOSC(FCKSM_CSECMD & OSCIOFNC_OFF);	// Enable Clock Switching and Output Clock to OSC2
_FWDT(FWDTEN_OFF); 					// Turn off Watchdog Timer
_FICD(JTAGEN_OFF & ICS_PGD2);		// Turn off JTAG and communicate on PGC1/EMUC1 and PGD1/EMUD1

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
void __attribute__((interrupt, no_auto_psv)) _T1Interrupt(void)
{
	IFS0bits.T1IF = 0;
    enc1pos = POS1CNT;
	wait_flag = 1; //Signal end of sample time	
	run_time++;
} // end T1Interupt


//////////////////////////////////////////////////////////
// Intialize Sample Time (equal to value of float samp_time)
void init_samptime(void)
{
	TMR1 = 0;
	PR1 = (samp_time*155) - 1; 	// Set the period register
	T1CON = 0x8030;		// TMR1 on, prescalar set to 1:8 Tclk/2
	_T1IF = 0; 			// Clear Flag
	_T1IE = 1;			// Enable Source
} //end init_samptime


//////////////////////////////////////////////////////////
//Initialize Pins
void init_pins(void)
/* Pin Overview Map
//		RA0 - AN0               (PIN19) input
//		RA1 - AN1       		(PIN20) input
//		RA2 - FREE              (PIN30)
//		RA3 - FREE              (PIN31)
//		RA4 - RGB green         (PIN34)
//		RA7 - Chip select 1     (PIN13)
//		RA8 - LED yellow        (PIN32)
//		RA9 - RGB Blue          (PIN35)
//		RA10 - Chip select 0    (PIN12)
//		RB0	- AN2				(PIN21) input
//		RB1	- AN3				(PIN22) input
//		RB2 - AN4               (PIN23) input
//		RB3 - AN5               (PIN24) input      
//		RB4 - RGB Red           (PIN33) 
//		RB5 - SCK2 (SPI2 master)(PIN41)
//		RB6 - SDI2 (SPI2 master)(PIN42) input
//		RB7 - SDO2 (SPI2 master)(PIN43)
//		RB8 - SCL1 (I2C)        (PIN44)
//		RB9 - SDA1 (I2C)        (PIN1)
//		RB10 - Program (PGED2)  (PIN8)
//		RB11 - Program (PGEC2)  (PIN9)
//		RB12 - INDX2 (ENC2)     (PIN10)	input
//		RB13 - QEB2 (ENC2)      (PIN11) input
//		RB14 - OC1   		    (PIN14)
//		RB15 - OC2		        (PIN15)     
//		RC0	- AN6				(PIN25) input
//		RC1	- AN7				(PIN26) input
//		RC2 - AN8               (PIN27) input
//		RC3 - SCK1 (SPI1 slave) (PIN36) input
//		RC4 - SDO1 (SPI1 slave) (PIN37)
//		RC5 - SDI1 (SPI1 slave) (PIN38) input
//		RC6 - QEA1 (ENC1)       (PIN2)  input
//		RC7 - QEB1 (ENC1)       (PIN3)  input
//		RC8 - INDX1 (ENC1)      (PIN4)  input
//		RC9 - QEA2 (ENC2)       (PIN5)  input
*/ 

{
	AD1PCFGL = 0x0;       // All analog pins set to analog

//    TRISA = 0x0000;     // analog set to digital out
	TRISA = 0x0003; 	// RA0,1 input, rest output
	ODCA = 0x00D0;			// RA4,8,9 open drain
	LATA = 0x0000;		// all pins set to zero

//    TRISB = 0x3040; 	// 	all analog set to digital out
	TRISB = 0x304F; 	// 	RB0,1,2,3,6,12,13 input,rest PORTB pins set to output
	ODCB = 0x0010;		//	RB4 open drain
	LATB = 0x0000;		//	all pins set to zero
    
//    TRISC = 0x03E8;     // analog set to digital out
    TRISC = 0x03EF; 	// RC0,1 input, rest output
	ODCC = 0;			// all pins set NOT open drain
	LATC = 0x0000;		// all pins set to zero
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
    RPOR10bits.RP20R = 7;       // Assign SDO1 to RP20, Pin 21 - OUTPUT
    RPINR20bits.SCK1R = 19;     // Assign SCK1 to RP19, Pin 23 - INPUT
    RPINR20bits.SDI1R = 21;     // Assign SDI1 to RP21, Pin 24 - INPUT 
    
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

}

/* 
 * Initializes Direct Memory Access
 * This allows the send and receive operations with the Playsurface to happen in the background 
 * while the CPU executes the main control loop.
 */
void init_dma0_spi1_tx(void)
{
    // Set up DMA Channel 0 for transmit in continuous ping-pong mode:
	DMA0CONbits.AMODE = 0;
	DMA0CONbits.MODE  = 3;
	DMA0CONbits.DIR   = 1;
	DMA0CONbits.SIZE  = 0; // Continuous, Post-Inc., RAM-to-peripheral
	DMACS0 = 0;
	DMA0CNT = RASPI_BUF_SIZE - 1; // 6 DMA requests 
	DMA0REQ = 0x000A; // Select Transfer done SPI1
	DMA0PAD = (volatile unsigned int) &SPI1BUF;
	DMA0STA = __builtin_dmaoffset(TxBufferA);
	DMA0STB = __builtin_dmaoffset(TxBufferB);
	IFS0bits.DMA0IF = 0;// Clear DMA interrupt
	IEC0bits.DMA0IE = 1;// Enable DMA interrupt
	DMA0CONbits.CHEN = 1;// Enable DMA Channel
}

void init_dma1_spi1_rx(void)
{
	// Set up DMA Channel 1 for receive in continuous ping-pong mode:
    DMA1CONbits.AMODE = 0;
	DMA1CONbits.MODE  = 2;
	DMA1CONbits.DIR   = 0;
	DMA1CONbits.SIZE  = 0; // continuous, post-inc, Periph-RAM
//	DMA1CONbits.SIZE = 1;
	DMA1CNT = RASPI_BUF_SIZE - 1; // 6 DMA requests
	DMA1REQ = 0x000A; // Select Transfer done SPI1
	DMA1PAD = (volatile unsigned int) &SPI1BUF;
	DMA1STA = __builtin_dmaoffset(RxBufferA);
	DMA1STB = __builtin_dmaoffset(RxBufferB);
	IFS0bits.DMA1IF = 0; // Clear DMA interrupt
	IEC0bits.DMA1IE = 1; // Enable DMA interrupt
	DMA1CONbits.CHEN = 1; // Enable DMA Channel
}
    
void init_spi1_buff(void)
{
unsigned int i;
    for(i=0;i<4;i++)
        TxBufferA[i]=i;
    for(i=0;i<3;i++)
        TxBufferB[i]=16+i;        
        
   	for(i=0;i<4;i++){
        RxBufferA[i]=0xDEED;
        RxBufferB[i]=0xDEED;
		}	
}

/*
 * Sets the SPI interrupt (communication completed)
 */
void __attribute__((interrupt, no_auto_psv)) _SPI1Interrupt(void)
{
	//spi1_flag++;
	IFS0bits.SPI1IF = 0;
//	receiveSPImsg();
	//SPI1STATbits.SPIROV = 0;
}

void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void)
{
	static unsigned int BufferCount = 0; // Keep record of the buffer that
	// contains TX data
	if(BufferCount == 0)
	{
		TxData(TxBufferA); // Transmit SPI data in
		// DMA RAM Primary buffer
	}
	else
	{
		TxData(TxBufferB); // Transmit SPI data in DMA RAM
		// Secondary buffer
	}
	BufferCount ^= 1;
    
    //while(SPI1STATbits.SPITBF);
    
    DMA0CONbits.CHEN = 1;
	IFS0bits.DMA0IF = 0; // Clear the DMA0 Interrupt flag
}

void __attribute__((interrupt, no_auto_psv)) _DMA1Interrupt(void)
{
	static unsigned int BufferCount = 0; // Keep record of the buffer
	// that contains RX data
	if(BufferCount == 0)
	{
		RxData(RxBufferA); // Process received SPI data in
		// DMA RAM Primary buffer
	}
	else
	{
		RxData(RxBufferB); // Process received SPI data in
		// DMA RAM Secondary buffer
	}
    

    
	BufferCount ^= 1;
	IFS0bits.DMA1IF = 0;
    SPI_wait_flag = 1;
}

//////////////////////////////////////////////////////////////////////////////
// PWM(Output Compare) Initialization
//////////////////////////////////////////////////////////////////////////////

/*
 * Interrupt on timer 2 to enforce the PWM pulse.
 */
void _ISR _T2Interrupt(void)
{
    OC1RS = pwm1_duty;
    OC2RS = pwm2_duty;
    IFS0bits.T2IF = 0;

} // end T2Interupt

/*
 * Initializes the PWM with Timer 2 and sets RP14 as the output pin
 */
void init_pwm1(void)
{
    //init TMR2
    T2CON = 0x8000; // prescalar 1:1
    _T2IF = 0; // Clear Flag
    _T2IE = 1; // Enable Source
    // init PWM1
    OC1CON = 0x0000; // disable PWM module
    TMR2 = 0; // Reset timer value
    PR2 = PERIOD-1; // set PWM period
    OC1RS = pwm1_duty; // set slave duty cycle
    OC1R = pwm1_duty; // duty cycle
    OC1CON = 0x0006; // activate PWM module
    RPOR7bits.RP14R = 18; // initialize RP14 to OC1 (PWM Output)
} // end init_pwm

void init_pwm2(void)
{
    //Optional - tie PWM2 to a separate timer from PWM1
    //init TMR3
//    T3CON = 0x8000; // prescalar 1:1
//    _T3IF = 0; // Clear Flag
//    _T3IE = 1; // Enable Source
    
    // init PWM2
    OC2CON = 0x0000; // disable PWM module
//    TMR2 = 0; // Reset timer value
    PR2 = PERIOD-1; // set PWM period
    OC2RS = pwm2_duty; // set slave duty cycle
    OC2R = pwm2_duty; // duty cycle
    OC2CON = 0x0006; // activate PWM module
    RPOR7bits.RP15R = 19; // initialize RP15 to OC2 (PWM Output)
}


/////////////////////////////////////////////////////////////
// Encoder module initialization
/////////////////////////////////////////////////////////////


//extern int enc1_res;
//extern int enc2_res;
void init_encoder1(void)
{
    //Setup remappable pins - Enc 1
    RPINR14bits.QEA1R = 22;
    RPINR14bits.QEB1R = 23;
    RPINR15bits.INDX1R = 24;
    
    //Initialize Encoder 1
    QEI1CONbits.QEISIDL = 0;    //Continue module operation in Idle mode
    QEI1CONbits.QEIM = 7;       //Set to 6 if using index pulse
    QEI1CONbits.SWPAB = 0;      //Phase A and Phase B inputs are not swapped
    QEI1CONbits.PCDOUT = 0;     //Position counter direction status output is disabled (normal I/O pin operation)
    QEI1CONbits.POSRES = 1;     //Index pulse resets position counter (only applies when QEIM = 4 or 6)
    DFLT1CONbits.IMV = 0;       //A and B input signals both 0 for match on index pulse
    DFLT1CONbits.QEOUT = 0;     //Digital filter outputs are disabled (normal pin operation)
    DFLT1CONbits.QECK = 3;      //1:16 Clock divide for QEA1/QEB1/INDX1
    POS1CNT = 0;
    MAX1CNT = ENC1_RES-1; 
    
    IFS3bits.QEI1IF = 0; // Clear DMA interrupt
	IEC3bits.QEI1IE = 1; // Enable DMA interrupt
}

extern int enc1revs;

void __attribute__((interrupt, no_auto_psv)) _QEI1Interrupt(void)
{
    if(QEI1CONbits.UPDN)
        enc1revs++;
    else
        enc1revs--;
    
    IFS3bits.QEI1IF = 0;
}

void init_encoder2(void)
{
    //Setup remappable pins - Enc 2
    RPINR16bits.QEA2R = 25;
    RPINR16bits.QEB2R = 13;
    RPINR17bits.INDX2R = 12;
    
    //Initialize Encoder 2
    QEI2CONbits.QEISIDL = 0;
    QEI2CONbits.QEIM = 7; //Set to 6 if using index pulse
    QEI2CONbits.SWPAB = 0;
    QEI2CONbits.PCDOUT = 0;
    QEI2CONbits.POSRES = 1;
    DFLT2CONbits.IMV = 0;
    DFLT2CONbits.QEOUT = 0;
    DFLT2CONbits.QECK = 3;
    POS2CNT = 0;
    MAX2CNT = ENC2_RES-1;
}

void initTmr3(void) 
{
        TMR3 = 0x0000;
        TMR1 = 0;
//        PR3 = 10000; 
        PR3 = 9999;         //Time out every 250 us (4khz)
        IFS0bits.T3IF = 0;
        IEC0bits.T3IE = 0;

        //Start Timer 3
        T3CONbits.TON = 1;

}

//void __attribute__((interrupt, no_auto_psv)) _T3Interrupt(void)
//{
////      = !MOT_D1;
//    IFS0bits.T3IF = 0;
//
//} // end T2Interupt

void init_ADC(void)
{
    
//    TMR3 = 0x0000;
//    PR3 = 4999;         //Time out every 125 us (8khz)
//    IFS0bits.T3IF = 0;
//    IEC0bits.T3IE = 0;
//
//    //Start Timer 3
//    T3CONbits.TON = 1;

    //Read all 9 analog inputs. 
    AD1CON1 = 0;
    AD1CON2 = 0;
    AD1CON3 = 0;
    AD1CON4 = 0;
    AD1CON1bits.SSRC = 2; // Internal counter ends sampling and starts conversion (auto-convert)
    AD1CON1bits.AD12B = 0; // Select 10-bit mode
    AD1CON2bits.CHPS = 3; // Select 4-channel mode
//    AD1CON2bits.CHPS = 0; // Convert only CH0
    AD1CON1bits.SIMSAM = 1; // Enable Simultaneous Sampling
    AD1CON2bits.ALTS = 1; // Enable Alternate Input Selection
//    AD1CON2bits.CSCNA = 1; // Enable Channel Scanning
//    AD1CON2bits.SMPI = 0; // Select 1 conversions between interrupt
    AD1CON2bits.SMPI = 1; // Select 2 conversions between interrupt
    AD1CON1bits.ASAM = 1; // Enable Automatic Sampling
    
    AD1CON3bits.ADRC = 0; // ADC Clock is derived from Systems Clock
//    AD1CON3bits.SAMC = 2; // Auto Sample Time = 2 * TAD
    AD1CON3bits.ADCS = 63; // ADC Conversion Clock Tad=Tcy*(ADCS+1)= (1/40M)*32 = 0.8us (1.25Mhz)
									// ADC Conversion Time for 10-bit Tc=12*Tab = 9.6us
    
    AD1CON1bits.ADDMABM = 0; 	// DMA buffers are built in scatter/gather mode
    AD1CON4bits.DMABL   = 2;	// Each buffer contains 4 words
    
    // Initialize Channel Scan Selection
//    AD1CSSLbits.CSS7 = 1; // Enable AN7 for scan
//    AD1CSSLbits.CSS8 = 1; // Enable AN8 for scan
    
    // Initialize MUXA Input Selection
    AD1CHS0bits.CH0SA = 6; // Select AN6 for CH0 +ve input
    AD1CHS0bits.CH0NA = 0; // Select VREF- for CH0 -ve inputs
    AD1CHS123bits.CH123SA = 0; // Select AN0,1,2 for CH1,2,3 +ve input
    AD1CHS123bits.CH123NA = 0; // Select Vref- for CH1,2,3 -ve inputs
    
    // Initialize MUXB Input Selection
    AD1CHS0bits.CH0SB = 7; // Select AN7 for CH0 +ve input
    AD1CHS0bits.CH0NB = 0; // Select VREF- for CH0 -ve inputs
    AD1CHS123bits.CH123SB = 1; // Select AN3,4,5 for CH1 +ve input
    AD1CHS123bits.CH123NB = 0; // Select VREF- for CH1 -ve inputs
    
    IFS0bits.AD1IF = 0; // Clear the Analog-to-Digital interrupt flag bit
    IEC0bits.AD1IE = 0; // Disable Analog-to-Digital interrupt 
    
    AD1CON1bits.ADON = 1; //Enable module
    
//	AD1CON1 = 0x00E0;
//	AD1CSSL = 0;
//	AD1CON2 = 0;
//	AD1CON3 = 0x1F02;
//	AD1CON1bits.ADON = 1;
}

void initDMA_ADC(void)
{
	// Initialize pin for toggling in DMA interrupt
	DMA2CONbits.AMODE = 2;			// Configure DMA for Register indirect with post increment
	DMA2CONbits.MODE  = 2;			// Configure DMA for Continuous Ping-Pong mode

	DMA2PAD=(int)&ADC1BUF0;
	DMA2CNT=(SAMP_BUFF_SIZE*NUM_ADC)-1;				
	
	DMA2REQ=13;	
	
	DMA2STA = __builtin_dmaoffset(ADCBufferA);		
	DMA2STB = __builtin_dmaoffset(ADCBufferB);

	IFS1bits.DMA2IF = 0;			//Clear the DMA interrupt flag bit
    IEC1bits.DMA2IE = 1;			//Set the DMA interrupt enable bit
	
	DMA2CONbits.CHEN=1;
}

unsigned int DmaBuffer = 0;
void __attribute__((interrupt, no_auto_psv)) _DMA2Interrupt(void)
{

		if(DmaBuffer == 0)
		{
			ProcessADCSamples(&ADCBufferA);
		}
		else
		{
			ProcessADCSamples(&ADCBufferB);

		}

		DmaBuffer ^= 1;

        IFS1bits.DMA2IF = 0;					 //Clear the DMA0 Interrupt Flag
}





void init_i2c(void)
{
    // I2C Master mode, clock = FOSC/(4 * (SSPADD + 1)) 
    I2C1BRG = 73;   		// 400Khz @ 80Mhz Fosc

	I2C1CONbits.I2CEN = 0;
	I2C1CONbits.I2CSIDL = 0;
	I2C1CONbits.SCLREL = 1;
	I2C1CONbits.IPMIEN = 0;
	I2C1CONbits.A10M = 0;
	I2C1CONbits.DISSLW = 1;
	I2C1CONbits.SMEN = 0;
	I2C1CONbits.GCEN = 0;
	I2C1CONbits.STREN = 0;
	I2C1CONbits.ACKDT = 0;
	I2C1CONbits.ACKEN = 0;
	I2C1CONbits.RCEN = 0;
	I2C1CONbits.PEN = 0;
	I2C1CONbits.RSEN = 0;
	I2C1CONbits.SEN = 0;
	//Clearing the recieve and transmit buffers
	I2C1RCV = 0x0000;
	I2C1TRN = 0x0000;
	
	//Enable the peripheral	
	I2C1CONbits.I2CEN = 1;
}


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
