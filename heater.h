// Header gaurd
#ifndef HEATER_H
#define HEATER_H


// Header files
#include <stdint.h>
#include "heater.h"


// Heater class
class Heater {

	// Public
	public :
	
		// Constructor
		Heater();
		
		// Set temperature
		void setTemperature(uint16_t value);
		
		// Get temperature
		int16_t getTemperature();
		
		// Turn off
		void turnOff();
	
	// Private
	private :
};


#endif
