/*******************************************************
Name: controllers.c
Date: 5/9/2016
Authors: Mark Price
Comments: Low to high level controllers for the haptic mouse
*******************************************************/

#include "p33FJ64MC202.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h" 		//Include Defitions and function prototypes
#include <string.h>
#include <stdlib.h>
#include "libpic30.h"//FCY must be defined before libpic30.h is included

// Include global variables

extern float p;
extern int pwm_duty;

/*
 * PWM Motor output controller
 * Sets the motor power to somewhere between -100% and +100%
 * Requires MOT_D1 and MOT_D2 to be the pins that are plugged into the h-bridge
 *  along with the pin that is generating the PWM wave
 *
 * Inputs:
 *      int V - the desired positive or negative percentage of power for the motor
 */
void setMotorPercent(float V)   // -10000<p<10000
{
	/*if (V < 30 && V > 0){ //Account for static friction dead zone
		V = 30;
	}
	if (V > -30 && V < 0){
		V = -30;
	}
	*/
//    p = V*(PERIOD/100)*-1.0; // PERIOD/100%
//    //p = -PERIOD*V/100;
//    if (p < 0)
//    {
//         if (p < -PERIOD) {
//             p = -PERIOD+1; // do not exceed 100% Duty
//         }
//         pwm_duty = -p; // convert sign and change direction
//         MOT_D1=1;delay();
//         MOT_D2=0;delay();
//    }
//    else {
//         if (p > PERIOD) p = PERIOD-1; // do not exceed 100% Duty
//         pwm_duty = p; // output duty cyle
//         MOT_D1=0;delay();
//         MOT_D2=1;delay();
//    }

}

////////////////////////////////////////////////////////
// POSITION/VELOCITY CONTROL
////////////////////////////////////////////////////////


/*
 * RAW ANGLE STEERING POSITION CONTROL - Using internal reference frame
 * 
 * PID steering angle controller. Best performance found with I term set to 0.
 * Steers "front" of wheel to desired angle (e.g. different response for 0 and 180 degrees)
 */

/*
void rawSteeringPosControl(float desAngle)
{
	// Set controller gains
	float Kp = .85;
	float Kd = .008;
	float Ki = 0;

	// Assign position error, translate to angle between -180 and 180 degrees
	transAngle = desAngle - steeringAngle;
	if (abs(transAngle) > 180){
		transAngle = transAngle - 360*fsign(transAngle);
	}

	// Sum integral error term, set limits
	errorSum = errorSum + (transAngle*period/1000);
	if (errorSum > 100){
		errorSum = 100;
	}
	else if (errorSum<-100){
		errorSum = -100;
	}

	float Iterm = errorSum*Ki;
	if (Iterm > 50){
		Iterm = 50;
	}
	else if (Iterm<-50){
		Iterm = -50;
	}
		
	// Set controller output in terms of PWM duty cycle.
	// Set ceilings so motor input never exceeds 100% (llows easier visualization of saturation)
	output = Kp*transAngle + Iterm - Kd*steeringSpeed;
	if(output > 100){
		output = 100;
	}
    else if(output < -100){
		output = -100;
	}

	// Compensate for motor deadzone
	if (abs(output) <= 1) output = 0;
	else if(output>1 && output <3) output = 3;
	else if(output<-1 && output > -3) output = -3;


	setMotorPercent(output);
	posControlFlag = 1;			// This flag goes high whenever position control is called. 
								// Used to ensure correct reference points for velocity control.
}
*/
