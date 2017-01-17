/*******************************************************
Name: serial.c
Date: 5/9/2016
Authors: Qiandong Nie, Mark Price
Comments: Serial communication functions (SPI, UART)
*******************************************************/
#include "p33FJ64MC204.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h"		//Include Defitions and function prototypes
#include "stdio.h"
#define FCY 40000000UL
#include "libpic30.h"		//Include Defitions and function prototypes
#include <stdlib.h>

extern int wait_flag;		// signals end of 1 millisecond sample period
extern int spi1_flag;		//Flag for SPI transmission
extern int spi2_flag;
extern int hallErrorFlag;
extern int alpha;
extern int vg;

extern int enc1pos;
extern unsigned int pwm1_duty_16bit;
extern int pwm1_duty;

extern unsigned int TxBufferA[RASPI_BUF_SIZE];
extern unsigned int TxBufferB[RASPI_BUF_SIZE];
extern unsigned int RxBufferA[RASPI_BUF_SIZE];
extern unsigned int RxBufferB[RASPI_BUF_SIZE];

extern unsigned int dataIn[RASPI_BUF_SIZE];
extern unsigned int dataOut[RASPI_BUF_SIZE];

//////////////////////////////////////////////////////////////////////////////
// SPI functions (RasPi - PIC)
//////////////////////////////////////////////////////////////////////////////

/*
 * Used to send constructed SPI messages
 *
 * Inputs:
 *      short SPItx_data - the value of the data to be sent
 */
short sendSpiMsgSlave(short SpiTxData)
{
    short SpiRxData = SPI1BUF;
    spi1_flag = 0;
    SPI1BUF = SpiTxData;
    while(spi1_flag<1);		//use Spi flag to indicate buffer is full
    return SpiRxData;
}

short sendSpiMsgMaster(short SpiTxData)
{
    spi1_flag = 0;
    SPI1BUF = SpiTxData;
    while(spi1_flag<1);		//use Spi flag to indicate buffer is full
    return SPI1BUF;
}

int msg_marker;
short enc1flag;
short oc1flag;
short rasPiErrFlag=0;

void TxData(unsigned int *TxBuff){

	int n;
    
    //spi1_flag = 0;
    int inputSize = sizeof(TxBufferA)/sizeof(TxBufferA[0]);

    
    for (n = 0; n < inputSize; n++){
        TxBuff[n] = dataOut[n];
    }
    
//    TxBuff[3] = 0x01;
//    TxBuff[0] = 0x00;
//    TxBuff[1] = 0x02;
//    TxBuff[2] = 0x33;
    
}

void RxData(unsigned int *RxBuff){

	int n;
    int inputSize = sizeof(RxBufferA)/sizeof(RxBufferA[0]);
    
    for (n = 0; n < inputSize; n++){
        dataIn[n] = RxBuff[n];
//        dataOut[n] = dataIn[n];
    }

}

void processRXdata(void){
    //MOT_D1 = 1;
    int n;
    int m;
    
    for (n = 0; n < RASPI_BUF_SIZE; n = n+3){
        for(m = 0; m < 3; m++){
            if(m == 0){         //low byte
                msg_marker = dataIn[n];
                dataOut[n] = msg_marker;
                switch(msg_marker){
                    case 1:
                        getEnc1Pos();                
                        break;
                    case 2:
                        pwm1_duty_16bit = ((dataIn[n+1]) | (dataIn[n+2] << 8)) & 0xffff;
                        pwm1_duty = pwm1_duty_16bit*BIT16_TO_MAX1000;
                        break;
                    default:
                        rasPiErrFlag = 1;
                        break;
                }     
            }
            else if (m == 1){
                switch(msg_marker){
                    case 1:
                        dataOut[n+m] = (enc1pos & 0x00FF);
                        break;
                    case 2:
                        dataOut[n+m] = dataIn[n+m];     //echo back commanded duty cycle
                        break;
                    default:
                        rasPiErrFlag = 1;
                        dataOut[n+m] = 0xFF;
                        break;
                }             
            }
            else{               // high byte
                switch(msg_marker){
                    case 1:
                        dataOut[n+m] = (enc1pos & 0xFF00) >> 8;
                        break;
                    case 2:
                        dataOut[n+m] = dataIn[n+m];     //echo back commanded duty cycle
                        break;
                    default:
                        rasPiErrFlag = 1;
                        dataOut[n+m] = 0xFF;
                        break;
                }             
            }

        }
    }
    //MOT_D1 = 0;
}

/*
void receiveSPImsg(void){
	dataIn =  SPI1BUF;
	SPI1BUF = dataIn;
}
*/
//////////////////////////////////////////////////////////////////////////////
// UART functions
//////////////////////////////////////////////////////////////////////////////


/*

 // printDMA - Transmits the DMA output buffer to the table via wired UART.

void printDMA(void){
	DMA0CONbits.CHEN = 1; // Enable DMA0 Channel
 	DMA0REQbits.FORCE = 1; // Manual mode: Kick-start the 1st transfer
}


// UARTreceiveDMA - Reads the DMA UART input buffer. Sets a flag if the transmission is 
// identical to the last received transmission (no new Playsurface IR camera information).

void UARTreceiveDMA(void){
	int i = 0;
	
	while (i<17){
		if(lastTransmit[i] != BufferB[i]){
			sameFlag = 0;
			tableData[i] = BufferB[i] & 0x00FF;
			lastTransmit[i] = BufferB[i] & 0x00FF;
		}
		i++;
	}
}



// UARTdecode_relative_bytes - Parses and assigns received bytes to internal variables. 
// For relative-to-obstacle coordinate system.

void UARTdecode_relative_bytes(void)
{
	appSelector = tableData[0];		// Table tells robot what mode to run in (solid obstacles vs pure path follow)
	pErr = (float)(tableData[1] | (tableData[2] << 8))/1000;
	velPar = (float)(tableData[3] | (tableData[4] << 8))/1000;
	velPerp = (float)(tableData[5] | (tableData[6] << 8))/1000;
	bodyTerr = (float)(tableData[7] | (tableData[8] << 8))/182;
	objectCompliance = (float)(tableData[9] | (tableData[10] << 8))/100;
	M = (float)(tableData[11] | (tableData[12] << 8))/100;
	dampRatio = (float)(tableData[13] | (tableData[14] << 8))/1000;
	objectCurvature = (float)(tableData[15] | (tableData[16] << 8))/(100);

	M = M*0.00259; // convert mass from lbm to lbf*s^2/in
	if (M > 0 && objectCompliance > 0) b = dampRatio*2*sqrt(objectCompliance*M);
	else b = 0;
}
*/
