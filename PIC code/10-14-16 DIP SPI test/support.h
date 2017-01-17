/*******************************************************
Name: support.h
Date: 2015
Authors: Mark Price
Comments:  Support file for definitions and function prototypes
*******************************************************/

#define PI 3.1415926
#define PERIOD 1000
#define LSB_TO_DEGREES 0.02197		//14 bit angular Hall effect sensor
//#define MOT_D1 _RA2                 // Motor Direction Input 1
//#define MOT_D2 _RA3
//#define NUM_ADC 8                   // # of ADC pins in use
//#define MAX_NUM_ADC 9               // Max # of ADC pins in use
//#define SAMP_BUFF_SIZE 8            // # of samples to hold for each AN pin
//#define ENC1_RES 2048               // # pulses x 4 (quadrature)
//#define ENC2_RES 2048
#define RASPI_BUF_SIZE 3            // Bytes in DMA buffer for SPI communication with Raspberry Pi

// Initialization Functions
void init_clock(void);
void init_samptime(void);
void init_pins(void);
void init_spi1(void);
void init_spi1_buff(void);
void init_dma0_spi1_tx(void);
void init_dma1_spi1_rx(void);
void init_spi_connection(int module);
void init_UART(void);
void init_pwm1(void);
void init_pwm2(void);
void init_spi2(void);
void init_encoder1(void);
void init_encoder2(void);
void initTmr3(void);
void init_ADC(void);
void initDMA_ADC(void);
void init_i2c(void);
//void ProcessADCSamples(int *AdcBuffer[MAX_NUM_ADC+1][SAMP_BUFF_SIZE]);

//UART Functions
void printLine(char first_line[], int l);
void UARTreceive(void);
void UARTdecode(void);
void UARTdecode_bytes(void);
void UARTdecode_relative_bytes(void);
void init_DMA(void);

//Motor Functions
void setMotorPercent(float p);
void delay(void);

//Encoder Functions
float getEnc1PosDeg(void);
float getEnc2PosDeg(void);
void getEnc1Pos(void);

//SPI Functions
//These functions send specific messages through SPI
unsigned int MemoryRead_msg(int module);
int NOP_COMMAND(int module);
unsigned int EEpromWrite_Message(int module);
unsigned int EERead_Challenge(int module);
unsigned int EEChallenge_Ans(int module);
unsigned int Reboot(int module);

float getEnc1PosDeg(void);
float getEnc2PosDeg(void);

short sendSpiMsgMaster(short SPItx_data);
short sendSpiMsgSlave(short SPItx_data);
void SpiDoGet(int module, unsigned short type);
void setLockSPIModule(int value, int module);
void getAngle(void);
void getForceMag(void);
int fsign(float num);
void freeMode(void);
void relFreeMode(void);
void freeModeNoTable(void);
void particleMode(void);
void relParticleMode(void);
void hardWall(void);
void sliderGame(void);
void steeringPosControl(float desAngle);
void steeringVelControl(float desVel);
void getTableData(void);
void getRobotData(void);
void getTableData_bytes(void);
void getRelativeTableData_bytes(void);
void getRelFakeTableData(void);
void linePathControl(int pathNumber);
void pathControl(void);
void circleDemo(void);
void compliantPath(void);
void rawSteeringPosControl(float desAngle);
void hapticPlayground(void);
void UARTreceiveDMA(void);
void printDMA(void);

void TxData(unsigned int *TxBuff);
void RxData(unsigned int *RxBuff);
void processRXdata(void);
