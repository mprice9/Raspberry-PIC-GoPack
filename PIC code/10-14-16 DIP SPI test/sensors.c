/*******************************************************
Name: sensors.c
Date: 5/9/2016
Authors: Mark Price
Comments: Functions supporting sensors and data acquisition
*******************************************************/

#include "p33FJ64MC202.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h" 		//Include Defitions and function prototypes
#include <string.h>
#include <stdlib.h>
#include "libpic30.h"//FCY must be defined before libpic30.h is included

extern float steeringAngle;
extern float fMag;
extern float fAngle;
extern double xDisp;
extern double yDisp;
extern double forceX;
extern double forceY;
extern int Bx;
extern int By;
extern int Bz;
extern unsigned int zVal;
extern unsigned int xVal;
extern unsigned int yVal;
extern unsigned int alpha;
extern float k;
extern int hallErrorFlag;

////////////////////////////////////////////////////////
// ENCODER FUNCTIONS
////////////////////////////////////////////////////////

//int min_tcount = 100;
//float getEnc1PosDeg(void){
//    float tcount = POS1CNT; // 360degrees = 2048 counts
//    float position = tcount*360/(ENC1_RES-1);
//    
//    if(tcount < min_tcount){
//        min_tcount = tcount;
//    }
//    return position;
//}
//
//int enc1pos = 0;
//void getEnc1Pos(void){
//    enc1pos = POS1CNT; // 360degrees = 2048 counts
//}
//
//float getEnc2PosDeg(void){
//    float tcount = POS2CNT; // 360degrees = 4096 counts
//    float position = tcount*360/(ENC2_RES-1);
//    return position;
//}
//
//
//////////////////////////////////////////////////////////
//// ADC FUNCTIONS
//////////////////////////////////////////////////////////
//
//void ProcessADCSamples(int * AdcBuffer[MAX_NUM_ADC+1][SAMP_BUFF_SIZE])
//{
//   // MOT_D1 = 1;
//    
//    ain0Buff[sampleCounter] = AdcBuffer[0][sampleCounter];
//    ain1Buff[sampleCounter] = AdcBuffer[1][sampleCounter];
//    ain2Buff[sampleCounter] = AdcBuffer[2][sampleCounter];
//    ain3Buff[sampleCounter] = AdcBuffer[3][sampleCounter];
//    ain4Buff[sampleCounter] = AdcBuffer[4][sampleCounter];
//    ain5Buff[sampleCounter] = AdcBuffer[5][sampleCounter];
//    ain6Buff[sampleCounter] = AdcBuffer[6][sampleCounter];
//    ain7Buff[sampleCounter] = AdcBuffer[7][sampleCounter];     
//    ain8Buff[sampleCounter] = AdcBuffer[8][sampleCounter];
//         
////    scanCounter++;
////	if(scanCounter==2){
////		scanCounter=0;
////	}
//
//    sampleCounter++;
//    
//	if(sampleCounter==SAMP_BUFF_SIZE){
//		sampleCounter=0;
//    }
//
//    IFS0bits.AD1IF = 0;
//
//    //MOT_D1 = 0;
//}
//
////int read_ADC(int scanCounter)
////{
////
////
////
////}
//
//////////////////////////////////////////////////////////
//// MLX90363 HALL EFECT SENSOR FUNCTIONS
//////////////////////////////////////////////////////////
///*
// * getAngle - Polls a Hall effect sensor for a steering angle measurement. Output is relative
// * to the robot reference frame (x - short axis, y - long axis. 0 degrees = aligned with positive x)
// */
//void getAngle(void)
//{
//	// Different set of commands required if sample time falls below approx 5 ms 
//	// (sensor won't hold previous measurement in memory between samples):
//	//			SpiDoGet_slow(2,0);
//	//			__delay_us(800);
//	//			NOP_COMMAND_slow(2);
//
//	// SPI exchange - GET command for Hall sensor
//	SpiDoGet(2,0);
//
//	// Convert angle from bytes to degrees
//	steeringAngle = alpha*LSB_TO_DEGREES;
//	delay();
//
//	// Offset for imperfect solder mount
//	if(steeringAngle < 355.6){
//		steeringAngle = steeringAngle + 4.4;
//	}
//	else{
//		steeringAngle = steeringAngle - 355.6;
//	}
//
//	// Reverse due to sensor being face-down
//	steeringAngle = 360 - steeringAngle;
//}
//
///*
// * getForceMag - Polls a Hall effect sensor for an input force vector measurement. Output is relative
// * to the robot reference frame (x - short axis, y - long axis. 0 degrees = aligned with positive x)
// */
//void getForceMag(void)
//{
//	// Different set of commands required if sample time falls below approx 5 ms 
//	// (sensor won't hold previous measurement in memory between samples):
//	//		SpiDoGet_slow(1,2);
//	//		__delay_us(800);
//	//		NOP_COMMAND_slow(1);
//
//	SpiDoGet(1,2);
//
//	// converts 12 bit sensor output to a signed int
//	if((xVal & 0x1000) != 0){
//		Bx = xVal - 0x1fff - 1;
//	}
//	else Bx = xVal & 0x0fff;
//	if((yVal & 0x1000) != 0){
//		By = yVal - 0x1fff - 1;
//	}
//	else By = yVal & 0x0fff;
//	if((zVal & 0x1000) != 0){
//		Bz = zVal - 0x1fff - 1;
//	}
//	else Bz = zVal & 0x0fff;
//
//
//	// Model magnetic flux in x,y,z to displacement in x and y
//	// Alternative models are commented out
//	if(Bz != 0){
////		xDisp = ((0.225*Bx)/(564 + Bz))-.0233;
//		xDisp = 2*((.0000217*Bx) + (((0.123*Bx) - 2.03 - (0.0033*By))/(120 + Bz)))-.0360;
////		xDisp = ((.0000301*Bx) + (((0.0979*Bx) - (0.00373*By))/Bz)) - .0207;
////		delay();
////		delay();
////		yDisp = (0.63*(0.225*By)/(564 + Bz))-.0104;
//		yDisp = 1.15*((.0000239*By) + ((0.208 + (0.117*By) + (0.00195*Bx))/(70.8 + Bz)))-.015;
////		yDisp = ((.0000305*By) + (((0.1*By) + (.00179*Bx))/Bz)) - .0118;
//
//	}
//	else{
//		xDisp = 0;
//		yDisp = 0;
//	}
//
//	// Calculate force based on flexure spring constant
//	forceX = k*xDisp;
//	forceY = k*yDisp;
//	fMag = sqrt((forceX*forceX) + (forceY*forceY));
//
//	if(yDisp > 0 && xDisp > 0){
//		fAngle = atan(yDisp/xDisp);
//	}
//	else if(yDisp > 0 && xDisp < 0){
//		fAngle = PI + atan(yDisp/xDisp);
//	}
//	else if(yDisp < 0 && xDisp < 0){
//		fAngle = PI + atan(yDisp/xDisp);
//	}
//	else if(yDisp < 0 && xDisp > 0){
//		fAngle = (2*PI) + atan(yDisp/xDisp);
//	}
//	else fAngle = 0;
//
//	fAngle = fAngle*180/PI;
//}
