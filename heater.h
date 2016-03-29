// Header gaurd
#ifndef HEATER_H
#define HEATER_H


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
