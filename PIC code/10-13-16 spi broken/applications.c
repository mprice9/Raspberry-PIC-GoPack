/*******************************************************
Name: applications.c
Date: 5/9/2016
Authors: Mark Price
Comments: Haptic mouse demos and applications. Most of these have been replaced in Unity.
*******************************************************/

#include "p33FJ64MC204.h" 	//Include p33FJ64MC202 header file
#include <math.h>			//Include math libary
#include "support.h" 		//Include Defitions and function prototypes
#include <string.h>
#include <stdlib.h>
#include "libpic30.h"//FCY must be defined before libpic30.h is included

extern float output;
extern float fMag;
extern float fAngle;
extern int Bz;
extern int hallErrorFlag;

extern int robotMode;
extern float tableX;
extern float tableY;
extern float velX;
extern float velY;
extern float tableAngle;
extern float velNorm;
extern float velAngle;
extern float velPerp;
extern float velPar;
extern float absSteeringAngle;
extern float absForceAngle;

extern char wallState;
extern float wallThreshold;
extern int dirCommit;
extern float xVelMovingAvg;
extern float yVelMovingAvg;
extern float oldXvel;
extern float oldYvel;
extern float absForceX;
extern float absForceY;
extern float pErr;
extern float tErr;
extern float oldPerr;
extern float oldTerr;
extern float pathCoords[];

extern float objectCurvature;
extern float bodyTerr;
extern float collisionAnticipationTime;
extern float fPerp;
extern int pathState;
extern float objectCurvature;
extern float centerX;
extern float centerY;
extern float relVelAngle;
extern float bodyTerr;
extern int dirSign;
extern float pAngle;



/*
 * HAPTIC PLAYGROUND - Renders path constraints as solid obstacles using a combination of
 * path-follow control and free mode. This is the only top level app still run on the PIC.
 *
 * Uses relative-to-obstacle coordinates.
 *
 * Intended for closed objects like circular objects. Requires some sign flipping in order 
 * to choose which side of a wall is solid.
 */
void hapticPlayground(void){

	// wallThreshold sets a distance from the boundary which starts the robot into a pre-collision 
	// steer parallel to the wall. This is set to correspond with a predicted time-to-collision threshold,
	// based on the velocity component normal to the path.
	wallThreshold = -velPerp * collisionAnticipationTime;
	if(wallThreshold < 0) wallThreshold = 0;


	// State controller:
	//		Approach mode (robot is within the collision threshold)
	//		Free mode (robot is outside the collision threshold or being pulled off the boundary)
	//		Path-follow mode (robot is on or inside the obstacle boundary)
	if((fPerp > 0.3)){

		if(wallState == 'a' || (wallState == 'p' && pathState == 0)){
			wallState = 'f';
		}
	}
	else if(pErr < 0.5){
		if(wallState == 'a' && pErr > 0) wallState = 'a';
		else wallState = 'p';
	}
	else if(pErr  < wallThreshold){
		wallState = 'a';
	}
	else wallState = 'f';

	
	switch(wallState){ 

		// Free mode
		case 'f':
			relParticleMode();
			//relFreeMode();
			break;

		// Approach mode: enforce a circular path tangent to the wheel and the wall. Output is steering velocity.
		case 'a':
			if(dirCommit == 0){
				output = -dirSign*velNorm*(1 - cos(tErr*PI/180))/(pErr);
			}
			else output = fsign(output)*abs(velNorm)*(1 - cos(tErr*PI/180))/(pErr);

			output = output*180/PI;
			steeringVelControl(output);

			if( abs(tErr) > 80) dirCommit = 1;	// Forces approach to commit to a direction for head-on collisions.
			else dirCommit = 0;

			break;

		// Path-follow mode
		case 'p':
			pathControl();
			break;
	}
}

///////////////////////////////////////////////////////////////
// OLD APPLICATIONS
///////////////////////////////////////////////////////////////

/*
 * VERTICAL MOVING WALL - Early test of force-detection-based mode switching.
 * Allows free motion to the right of the device (short axis, front end facing forward). Blocks motion to the left.
 */
void vertMovingWall(void)
{
	getForceMag();
	delay();
	delay();

	if (fMag > .4 && hallErrorFlag == 0 && Bz > 0x0850){
		if (fAngle < 90 || fAngle > 270){
			getAngle();
			freeMode();
		}
		else steeringPosControl(90.0);
	}
	else setMotorPercent(0);
}

/*
 * HARD WALL - Wall collision simulation. Wall position is hardcoded, uses global xy coordinates 
 * provided by Playsurface. hapticPlayground() offers a generalized version of this application.
 */
void hardWall(void)
{
	//'f' for free, 'p' for path, 'a' for approach

	// Wall threshold as calculated in hapticPlayground().
	wallThreshold = -xVelMovingAvg*.4 + 10;
	if(wallThreshold < 10) wallThreshold = 0;

	// Path/Approach/Free modes as defined in hapticPlayground(), specific to a vertical wall 10 
	// inches right of the left edge of the screen.
	if((absForceX > 0.3)){
		if(wallState == 'a' || (wallState == 'p' && pathState == 0)){
			wallState = 'f';
		}
	}
	else if(tableX < 10.5){
		wallState = 'p';
		if(wallState == 'a' && tableX > 10) wallState = 'a';
	}
	else if(tableX  < wallThreshold){
		wallState = 'a';
	}
	else wallState = 'f';

	
	switch(wallState){ 
		case 'f':
			//particleMode();
			freeMode();
			break;
		case 'a':
			tErr = absSteeringAngle - 90;
			if (abs(tErr) > 180){
				tErr = tErr - 360*fsign(tErr);
			}
	
			if (abs(tErr) > 90){
				tErr = tErr - 180*fsign(tErr); 
			}


			if(dirCommit == 0){
				output = -fsign(tErr)*velNorm*(1 - cos(tErr*PI/180))/(tableX - 10);
			}
			else output = fsign(output)*velNorm*(1 - cos(tErr*PI/180))/(tableX - 10);

			output = output*180/PI;
			steeringVelControl(output);

			if( abs(tErr) > 80) dirCommit = 1;
			else dirCommit = 0;

			break;
		case 'p':
			linePathControl(1);
			break;

	}
}

extern int pathNum;

/*
 * SLIDER GAME - Constrains the robot to 4 intersecting linear paths at right angles.
 * Path positions are hardcoded in pathCoords[]. This function was replaced by a Playsurface app, 
 * PIC now just runs pathControl() with inputs provided by table.
 */
void sliderGame(void)
{
	
	if(abs(tableX  - pathCoords[0]) < 1 && pathNum != 2) pathNum = 1;
	else if(abs(tableY - pathCoords[1]) < 1 && tableX  > pathCoords[0] && pathNum != 1 && pathNum != 3) pathNum = 2;	
	else if(abs(tableX  - pathCoords[2]) < 1 && pathNum != 2 && pathNum != 4) pathNum = 3;
	else if(abs(tableY - pathCoords[3]) < 1 && tableX  > pathCoords[2] && pathNum != 3) pathNum = 4;


	switch(pathNum){
		case 1:
			if((abs(tableY - pathCoords[1]) < 1) && (fMag > 0.6) && (absForceAngle < 40 || absForceAngle > 320) && (abs(velY) < 8)){
				pathNum = 2;
			}
			break;
		case 2:
			if(tableX  < pathCoords[0] || ((tableX  - pathCoords[0] < 1) && ((fMag > 0.6) && ((abs(absForceAngle - 90) < 40) || ((abs(absForceAngle - 270)) < 40))))){
				pathNum = 1;
			}
			else if((abs(tableX  - pathCoords[2]) < 1)&& (fMag > 0.6) && ((abs(absForceAngle - 90) < 40) || ((abs(absForceAngle - 270)) < 40)) && (abs(velX) < 8)){
				pathNum = 3;
			}
			break;
		case 3:
			if((abs(tableY - pathCoords[1]) < 1)&& (fMag > 0.6) && ((absForceAngle < 40 || absForceAngle > 320) || ((abs(absForceAngle - 180)) < 40)) && (abs(velY) < 8)){
				pathNum = 2;
			}
			else if((abs(tableY - pathCoords[3]) < 1)&& (fMag > 0.6) && (absForceAngle < 40 || absForceAngle > 320) && (abs(velY) < 8)){
				pathNum = 4;
			}
			break;
		case 4:
			if(tableX  < pathCoords[2] || ((tableX  - pathCoords[2] < 1)&& ((fMag > 0.6) && ((abs(absForceAngle - 90) < 40) || ((abs(absForceAngle - 270)) < 40))))){
				pathNum = 3;
			}
			break;
	}
	linePathControl(pathNum);
}

/*
 * SLIDER GAME HARD - Experimenting with higher force thresholds to switch paths at intersections. 
 * Attempt to simulate "breakthrough" or pushing through a stiff gate.
 */
void sliderGameHard(void)
{
	//Get abs position
	//Steer onto first path
	//Path control, vertical
	getRobotData();
	

	if(abs(tableX  - pathCoords[0]) < 1 && pathNum != 2) pathNum = 1;
	else if(abs(tableY - pathCoords[1]) < 1 && tableX  > pathCoords[0] && pathNum != 1 && pathNum != 3) pathNum = 2;	
	else if(abs(tableX  - pathCoords[2]) < 1 && pathNum != 2 && pathNum != 4) pathNum = 3;
	else if(abs(tableY - pathCoords[3]) < 1 && tableX  > pathCoords[2] && pathNum != 3) pathNum = 4;


	switch(pathNum){
		case 1:
			if((abs(tableY - pathCoords[1]) < 1) && (fMag > 1.6) && (absForceAngle < 40 || absForceAngle > 320) && (abs(velY) < 8)){
				pathNum = 2;
			}
			break;
		case 2:
			if(tableX  < pathCoords[0] || ((tableX  - pathCoords[0] < 1) && ((fMag > 1.6) && ((abs(absForceAngle - 90) < 40) || ((abs(absForceAngle - 270)) < 40))))){
				pathNum = 1;
			}
			else if((abs(tableX  - pathCoords[2]) < 1)&& (fMag > 1.6) && ((abs(absForceAngle - 90) < 40) || ((abs(absForceAngle - 270)) < 40)) && (abs(velX) < 8)){
				pathNum = 3;
			}
			break;
		case 3:
			if((abs(tableY - pathCoords[1]) < 1)&& (fMag > 1.6) && ((absForceAngle < 40 || absForceAngle > 320) || ((abs(absForceAngle - 180)) < 40)) && (abs(velY) < 8)){
				pathNum = 2;
			}
			else if((abs(tableY - pathCoords[3]) < 1)&& (fMag > 1.6) && (absForceAngle < 40 || absForceAngle > 320) && (abs(velY) < 8)){
				pathNum = 4;
			}
			break;
		case 4:
			if(tableX  < pathCoords[2] || ((tableX  - pathCoords[2] < 1)&& ((fMag > 1.6) && ((abs(absForceAngle - 90) < 40) || ((abs(absForceAngle - 270)) < 40))))){
				pathNum = 3;
			}
			break;
	}
	linePathControl(pathNum);
}

/*
 * CIRCLE DEMO - Simulates relative-to-obstacle coordinates from table before they were implemented in Unity.
 * Constrains robot to the perimeter of a 5 in radius circle.
 */
void circleDemo(void){

	// Calculate displacement error from global xy coordinates.
	pErr = sqrt((tableX - centerX)*(tableX - centerX) + (tableY - centerY)*(tableY - centerY)) - (1/objectCurvature);
	float yDistToCenter = tableY - centerY;
	float xDistToCenter = tableX - centerX;

	// Calculate angular position of robot relative to circle center.
	if(yDistToCenter >= 0 && xDistToCenter >= 0){
		pAngle = 2*PI + atan(-xDistToCenter/yDistToCenter);
	}
	else if(yDistToCenter  >= 0 && xDistToCenter < 0){
		pAngle = atan(-xDistToCenter/yDistToCenter);
	}
	else if(yDistToCenter  < 0 && xDistToCenter < 0){
		pAngle = PI + atan(-xDistToCenter/yDistToCenter);
	}
	else if(yDistToCenter  < 0 && xDistToCenter >= 0){
		pAngle = (PI) + atan(-xDistToCenter/yDistToCenter);
	}
	else pAngle = 0;

	pAngle = pAngle*180/PI;

	relVelAngle = velAngle - 90 - pAngle;
	if (abs(relVelAngle) > 180){
		relVelAngle = relVelAngle - 360*fsign(relVelAngle);
	}

	// Sets path follow direction sign based on clockwise/counterclockwise movement of robot around circle.
	dirSign = fsign(relVelAngle);

	// Calculates body angle relative to path tangent from absolute orientation.
	float tangentAngle = pAngle + 90;
	if (tangentAngle > 360) tangentAngle = tangentAngle - 360;
	bodyTerr = tangentAngle - tableAngle;

	pathControl();
	//compliantPath();
}





