// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "gcode.h"


// Definitions
enum MODES {RELATIVE, ABSOLUTE};
enum STEPS {STEP8 = 8, STEP16 = 16, STEP32 = 32};


// Motors class
class Motors {

	// Public
	public :
	
		// Constructor
		Motors();
		
		// Set micro steps per step
		void setMicroStepsPerStep(STEPS step);
		
		// Turn on
		void turnOn();
		
		// Turn off
		void turnOff();
		
		// Set mode
		void setMode(MODES mode);
		
		// Move
		void move(const Gcode &command);
	
	// Private
	private :
	
		// Mode
		MODES mode;
		
		// Steps
		STEPS step;
		
		// Current values
		float currentX;
		float currentY;
		float currentZ;
		float currentE;
		float currentF;
};


#endif
