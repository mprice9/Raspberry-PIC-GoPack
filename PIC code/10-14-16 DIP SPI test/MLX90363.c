/*******************************************************
Name: MLX90363.c
Date: 5/9/2016
Authors: Qiandong Nie, Mark Price
Comments: SPI commands for triaxis Hall effect ICs
*******************************************************/
#include "p33FJ64MC202.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h"		//Include Defitions and function prototypes
#include "stdio.h"
#include "libpic30.h"		//Include Defitions and function prototypes
#include <stdlib.h>

extern int wait_flag;		// signals end of 1 millisecond sample period
extern int spi1_flag;		//Flag for SPI transmission
extern int spi2_flag;
extern int hallErrorFlag;
extern int alpha;
extern int vg;
extern int CRCthing;

//////////////////////////////////////////////////////////
// SPI Melexis MLX90363 Hall Effect Sensor Functions
//////////////////////////////////////////////////////////


/*
 * Array for building SPI message CRC
 */
unsigned short cba_256_TAB[] = {
    	//0     1    2      3     4    5     6     7
    	0x00, 0x2f, 0x5e, 0x71, 0xbc, 0x93, 0xe2, 0xcd,  // 0
   	    0x57, 0x78, 0x09, 0x26, 0xeb, 0xc4, 0xb5, 0x9a,  // 1
    	0xae, 0x81, 0xf0, 0xdf, 0x12, 0x3d, 0x4c, 0x63,  // 2
    	0xf9, 0xd6, 0xa7, 0x88, 0x45, 0x6a, 0x1b, 0x34,  // 3
   		0x73, 0x5c, 0x2d, 0x02, 0xcf, 0xe0, 0x91, 0xbe,  // 4
    	0x24, 0x0b, 0x7a, 0x55, 0x98, 0xb7, 0xc6, 0xe9,  // 5
    	0xdd, 0xf2, 0x83, 0xac, 0x61, 0x4e, 0x3f, 0x10,  // 6
    	0x8a, 0xa5, 0xd4, 0xfb, 0x36, 0x19, 0x68, 0x47,  // 7
    	0xe6, 0xc9, 0xb8, 0x97, 0x5a, 0x75, 0x04, 0x2b,  // 8
    	0xb1, 0x9e, 0xef, 0xc0, 0x0d, 0x22, 0x53, 0x7c,  // 9
    	0x48, 0x67, 0x16, 0x39, 0xf4, 0xdb, 0xaa, 0x85,  // 10
    	0x1f, 0x30, 0x41, 0x6e, 0xa3, 0x8c, 0xfd, 0xd2,  // 11
    	0x95, 0xba, 0xcb, 0xe4, 0x29, 0x06, 0x77, 0x58,  // 12
    	0xc2, 0xed, 0x9c, 0xb3, 0x7e, 0x51, 0x20, 0x0f,  // 13
    	0x3b, 0x14, 0x65, 0x4a, 0x87, 0xa8, 0xd9, 0xf6,  // 14
    	0x6c, 0x43, 0x32, 0x1d, 0xd0, 0xff, 0x8e, 0xa1,  // 15
    	0xe3, 0xcc, 0xdb, 0x92, 0x5f, 0x70, 0x01, 0x2e,  // 16
        0xb4, 0x9b, 0xea, 0xc5, 0x08, 0x27, 0x56, 0x79,  // 17
        0x4d, 0x62, 0x13, 0x3c, 0xf1, 0xde, 0xaf, 0x80,  // 18
        0x1a, 0x35, 0x44, 0x6b, 0xa6, 0x89, 0xf8, 0xd7,  // 19
        0x90, 0xbf, 0xce, 0xe1, 0x2c, 0x03, 0x72, 0x5d,  // 20
        0xc7, 0xe8, 0x99, 0xb6, 0x7b, 0x54, 0x25, 0x0a,  // 21
        0x3e, 0x11, 0x60, 0x4f, 0x82, 0xad, 0xdc, 0xf3,  // 22
        0x69, 0x46, 0x37, 0x18, 0xd5, 0xfa, 0x8b, 0xa4,  // 23
        0x05, 0x2a, 0x5b, 0x74, 0xb9, 0x96, 0xe7, 0xc8,  // 24
        0x52, 0x7d, 0x0c, 0x23, 0xee, 0xc1, 0xb0, 0x9f,  // 25
        0xab, 0x84, 0xf5, 0xda, 0x17, 0x38, 0x49, 0x66,  // 26
        0xfc, 0xd3, 0xa2, 0x8d, 0x40, 0x6f, 0x1e, 0x31,  // 27
        0x76, 0x59, 0x28, 0x07, 0xca, 0xe5, 0x94, 0xbb,  // 28
        0x21, 0x0e, 0x7f, 0x50, 0x9d, 0xb2, 0xc3, 0xec,  // 29
        0xd8, 0xf7, 0x86, 0xa9, 0x64, 0x4b, 0x3a, 0x15,  // 30
        0x8f, 0xa0, 0xd1, 0xfe, 0x33, 0x1c, 0x6d, 0x42   // 31
    };


////////////////////////////////////////
////////////////////////////////////////
	extern unsigned int zVal;
	extern unsigned int xVal;
	extern unsigned int yVal;
	unsigned int data1;
	unsigned int data2;
	unsigned int ix;
    unsigned int    txferSize;
  	unsigned short* pTxBuff;
   	unsigned short* pRxBuff;
	unsigned short crc;
    	unsigned short byte0;
        unsigned short byte1;
        unsigned short byte2;
        unsigned short byte3;
        unsigned short byte4;
        unsigned short byte5;
        unsigned short byte6;

	unsigned short CODE;
	unsigned short Opcode14;

/*
 * EEpromWrite_Message - EEPROM write command to Hall effect sensor - sets sensor gain, filtering options, etc
 * int module refers to the sensor being polled: 1 = force sensor, 2 = steering angle sensor.
 */
unsigned int EEpromWrite_Message(int module)
{

    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        // Construct EepromWrite command
        pTxBuff[0] = 0x2E00; // VIRTUAL GAIN ADDRESS
        pTxBuff[1] = 0xBECB; // KEY
        pTxBuff[2] = 0x0303; //Date Word (Set the gain as 10)
        pTxBuff[3] = 0b11000011; //

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);

        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;
    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}



unsigned short CHKEY;
unsigned short Opcode4;
unsigned short Challenge_code;

/*
 * EERead_Challenge - Follows EEPROM write command, receive challenge code
 */
unsigned int EERead_Challenge(int module)
{
    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        // Construct GET1 command
        pTxBuff[0] = 0b0000000000000000; // None
        pTxBuff[1] = 0b0000000000000000; // None
        pTxBuff[2] = 0b0000000000000000; // None
        pTxBuff[3] = 0b11001111; // Opcode 15

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;
		CHKEY = (pRxBuff[1] >> 8 ) | ((pRxBuff[1] & 0xFF) << 8);	
		Opcode4 = (pRxBuff[3] >> 8 ) | ((pRxBuff[3] & 0xFF) << 8);
		Challenge_code = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0xFF) << 8);	
    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}


unsigned short Opcode40;
unsigned short Key_Echo;
unsigned short Inverted_Key_Echo;
unsigned short crc_recoder;


/*
 * EEChallenge_Ans - Respond with proper key sequence after EERead_Challenge. See MLX90363 datasheet for more info.
 */
unsigned int EEChallenge_Ans(int module)
{
    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        // Construct GET1 command
        pTxBuff[0] = 0b0000000000000000; // None
        pTxBuff[1] = CHKEY ^ 0x1234; // Key Echo
		Key_Echo = CHKEY ^ 0x1234;// To use a unique recoder to present Key Echo
		Inverted_Key_Echo = ~Key_Echo;// To use a unique recoder to present Inverted Key Echo
        pTxBuff[2] = Inverted_Key_Echo; //Inverted Key Echo
        pTxBuff[3] = 0b11000101; // Opcode 5

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;
		crc_recoder = crc; 		// present crc
        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8); 	// Junk values
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); 	// Key Echo	
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8);	// Inverted Key Echo
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); 	// Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;
		Opcode40 = (pRxBuff[3] >> 8 ) | ((pRxBuff[3] & 0xFF) << 8);	
	


    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}


/*
 * MemoryRead_msg - Sent in Hall effect chip initialization phase. Received data is not used.
 */
unsigned int MemoryRead_msg(int module)
{

    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        
        pTxBuff[0] = 0x102E; // ADDR1  0x102E
        pTxBuff[1] = 0x1032; // ADDR2  0x1032
        pTxBuff[2] = 0; //Junk values
        pTxBuff[3] = 0b11000001; //

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;
		data1 = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0xFF) << 8);	
		data2 = (pRxBuff[1] >> 8 ) | ((pRxBuff[1] & 0xFF) << 8);


    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}


/*
 * NOP_COMMAND - Filler command used to receive data that was not received on the request command
 */
int NOP_COMMAND(int module)
{
    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        // Construct NOP command
        pTxBuff[0] = 0; // None
        pTxBuff[1] = 0b0101010101010101; // Key (it doesn't matter)
        pTxBuff[2] = 0; //Junk values
        pTxBuff[3] = 0b11010000; //

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;
		CODE = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0xFF) << 8);	
		Opcode14 = (pRxBuff[3] >> 8 ) | ((pRxBuff[3] & 0xFF) << 8);


    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}

/*
 * NOP_COMMAND_slow - Filler command used to receive sensor magnetic field data for sample times 
 * above approx 5 ms. For polling periods this long, the sensor does not retain data in memory 
 * between samples, so each request must be followed about 800 us later with this function to receive the data.
 */
int NOP_COMMAND_slow(int module)
{
    txferSize=4;

	int type;
	if(module == 1) type=2;
	else type = 0;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        // Construct NOP command
        pTxBuff[0] = 0; // None
        pTxBuff[1] = 0b0101010101010101; // Key (it doesn't matter)
        pTxBuff[2] = 0; //Junk values
        pTxBuff[3] = 0b11010000; //

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;

				hallErrorFlag = (pDst[0] >> 6 & 0x03) <= 1;
        if((pDst[3] >> 14) == type)
	{
            //PORTBbits.RB4 = !PORTBbits.RB4;
			// The message is the returned message
            if (((pDst[0] >> 6) & 0x03) > 1)			// diagnostic pass
            {
				PORTBbits.RB4 = 1;
				if(type == 2)
				{
	                xVal = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0x1F) << 8);		// 12 bit resolution
	                yVal = (pRxBuff[1] >> 8 ) | ((pRxBuff[1] & 0x1F) << 8);
	                zVal = (pRxBuff[2] >> 8 ) | ((pRxBuff[2] & 0x1F) << 8);
				}
				else if(type == 0)
				{
	                alpha = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0x3F) << 8);		// 14 bit resolution
	                vg = (pRxBuff[2] >> 8);
				}
            }
        
			else setMotorPercent(0);
		}
    
	//	CODE = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0xFF) << 8);	
	//	Opcode14 = (pRxBuff[3] >> 8 ) | ((pRxBuff[3] & 0xFF) << 8);


    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}

/*
 * Reboot - Forces the Hall effect sensor to restart.
 */
unsigned int Reboot(int module)
{
    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;

        // Construct GET1 command
        pTxBuff[0] = 0b0000000000000000; // Junk values
        pTxBuff[1] = 0b0000000000000000; // Junk values
        pTxBuff[2] = 0; //Junk values
        pTxBuff[3] = 0b11101111; // opcode

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];

        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

                        
        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;
		


    }
    else
    { // Memory allocation failed

    }

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

    return zVal;
}





/*
 * This function is used to read from the sensor conected to through SPI
 *
 * Inputs:
 *      int module - which SPI sensor you are reading from (1 = force, 2 = angle)
 *		unsigned short type - which data you are requesting from the sensor (0 for angle, 2 for Bx, By, Bz)
 *
 * Outputs:
 * 		if type = 1:
 *		
 * 		extern int alpha is updated with the steering angle reading in bytes
 * 		extern int vg is updated with the sensor gain value
 * 
 *      if type = 2:
 *
 *      extern unsigned ints xVal, yVal, zVal are updated with the x,y,z magnetic field readings in bytes
 *
 */

void SpiDoGet(int module,unsigned short type)
{
    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;
		
		
        // Construct GET1 command
        pTxBuff[0] = 0; // Do not reset rolling counter
        pTxBuff[1] = 0b1111111111111111; // Time-out value
        pTxBuff[2] = 0; //Junk values
		if(type == 2){
        pTxBuff[3] = 0b10010011; //
		}
		else {
		pTxBuff[3] = 0b00010011;
		}

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];
		
		
        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
		
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;

		hallErrorFlag = (pDst[0] >> 6 & 0x03) <= 1;
        if((pDst[3] >> 14) == type)
	{
            //PORTBbits.RB4 = !PORTBbits.RB4;
			// The message is the returned message
            if (((pDst[0] >> 6) & 0x03) > 1)			// diagnostic pass
            {
				//PORTBbits.RB4 = 1;
				if(type == 2)
				{
	                xVal = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0x1F) << 8);		// 12 bit resolution
	                yVal = (pRxBuff[1] >> 8 ) | ((pRxBuff[1] & 0x1F) << 8);
	                zVal = (pRxBuff[2] >> 8 ) | ((pRxBuff[2] & 0x1F) << 8);
				}
				else if(type == 0)
				{
	                alpha = (pRxBuff[0] >> 8 ) | ((pRxBuff[0] & 0x3F) << 8);		// 14 bit resolution
	                vg = (pRxBuff[2] >> 8);
				}
            }
        
			else setMotorPercent(0);
		}
    }


    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

}

/*
 * Same as SpiDoGet, for use when sample time is above approx 5 ms to be followed by NOP_COMMAND_slow.
 */
void SpiDoGet_slow(int module,unsigned short type)
{
    txferSize=4;

    pTxBuff=(unsigned short*)malloc(txferSize*sizeof(short));
    pRxBuff=(unsigned short*)malloc(txferSize*sizeof(short));   // We'll transfer 16 bits words

	//PORTBbits.RB4 != PORTBbits.RB4;
    if(pTxBuff && pRxBuff)
    {
        unsigned short* pSrc = pTxBuff;
        unsigned short* pDst = pRxBuff;
        int ix;
        unsigned short rdData;
		
		
        // Construct GET1 command
        pTxBuff[0] = 0; // Do not reset rolling counter
        pTxBuff[1] = 0b1111111111111111; // Time-out value
        pTxBuff[2] = 0; //Junk values
		if(type == 2){
        pTxBuff[3] = 0b10010011; //
		}
		else {
		pTxBuff[3] = 0b00010011;
		}

         byte0 = pTxBuff[0] & 0xFF;
         byte1 = pTxBuff[0] >> 8;
         byte2 = pTxBuff[1] & 0xFF;
         byte3 = pTxBuff[1] >> 8;
         byte4 = pTxBuff[2] & 0xFF;
         byte5 = pTxBuff[2] >> 8;
         byte6 = pTxBuff[3] & 0xFF;

        crc = 0xFF;
        crc = cba_256_TAB[byte0 ^ crc];
        crc = cba_256_TAB[byte1 ^ crc];
        crc = cba_256_TAB[byte2 ^ crc];
        crc = cba_256_TAB[byte3 ^ crc];
        crc = cba_256_TAB[byte4 ^ crc];
        crc = cba_256_TAB[byte5 ^ crc];
        crc = cba_256_TAB[byte6 ^ crc];
        crc = ~crc & 0xFF;

        pTxBuff[3] = (crc << 8) | pTxBuff[3];
		
		
        // Construct NOP command with most significant bit first
        pTxBuff[0] =  ((pTxBuff[0] & 0xFF) << 8) | (pTxBuff[0] >> 8);//0; // Do not reset rolling counter
        pTxBuff[1] =  ((pTxBuff[1] & 0xFF) << 8) | (pTxBuff[1] >> 8); // Time-out value
        pTxBuff[2] =  ((pTxBuff[2] & 0xFF) << 8) | (pTxBuff[2] >> 8); //Junk values
        pTxBuff[3] =  ((pTxBuff[3] & 0xFF) << 8) | (pTxBuff[3] >> 8); // Opcode

        pTxBuff[3] = ((pTxBuff[3] & 0xFF00) | (crc & 0xFF));

        ix=txferSize; // Transfer one extra word to give the slave the possibility to reply back the last sent word

        while(ix--)
        {
            setLockSPIModule(0, module);
            unsigned short sendValue = *pSrc++;
            rdData = sendSpiMsgMaster(sendValue);
            *pDst++=rdData; // Store the received data
        }
        setLockSPIModule(1, module);
		
        // Now let's check that the data was received ok
        pSrc=pTxBuff;
        pDst=pRxBuff;		
	}

    free(pTxBuff);  // Free the allocated buffers
    free(pRxBuff);

}


/*
 * This function is used to set the value of the latch to allow SPI transmissions
 *
 * Inputs:
 *      int value - 0 or 1, the value that the latch is set to
 *      int module - 1 or 2, the SPI module that you are using
 */
void setLockSPIModule(int value, int module)
{
    if(module == 1)
        LATBbits.LATB7 = value;                // Lower MLX slave select line to initiate data exchange
    else
        LATBbits.LATB11 = value;
}

