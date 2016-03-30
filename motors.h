// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "gcode.h"


// Definitions
enum MODES {RELATIVE, ABSOLUTE};
enum STEPS {STEPS8, STEPS16, STEPS32};


// Motors class
class Motors {

	// Public
	public :
	
		// Constructor
		Motors();
		
		// Set micro steps per step
		void setMicroStepsPerStep(STEPS steps);
		
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
		
		// Current values
		float currentX;
		float currentY;
		float currentZ;
		float currentE;
		float currentF;
};


#endif
