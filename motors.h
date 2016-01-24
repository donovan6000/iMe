// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include <stdint.h>
#include "gcode.h"


enum MODES {RELATIVE, ABSOLUTE};


// Motors class
class Motors {

	// Public
	public :
	
		// Constructor
		Motors();
		
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
		
		// Motors VREF
		pwm_config motorXVrefPwm;
		pwm_config motorYVrefPwm;
		pwm_config motorZVrefPwm;
		pwm_config motorEVrefPwm;
};


#endif
