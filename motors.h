// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "gcode.h"


// Definitions
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_VREF_TIMER_PERIOD 0x27F

enum MODES {RELATIVE, ABSOLUTE};
enum STEPS {STEP8 = 8, STEP16 = 16, STEP32 = 32};


// Motors class
class Motors {

	// Public
	public :
	
		// Initialize
		void initialize();
		
		// Set micro steps per step
		void setMicroStepsPerStep(STEPS step);
		
		// Turn on
		void turnOn();
		
		// Turn off
		void turnOff();
		
		// Move
		void move(const Gcode &command);
		
		// Go home
		void goHome();
		
		// Set Z to zero
		void setZToZero();
		
		// Emergency stop
		void emergencyStop();
		
		// Current values
		float currentX;
		float currentY;
		float currentZ;
		float currentE;
		float currentF;
		
		// Mode
		MODES mode;
		
		// Speed limits
		float motorXSpeedLimit;
		float motorYSpeedLimit;
		float motorZSpeedLimit;
		float motorESpeedLimitExtrude;
		float motorESpeedLimitRetract;
	
	// Private
	private :
		
		// Steps
		STEPS step;
};


#endif
