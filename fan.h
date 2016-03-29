// Header gaurd
#ifndef FAN_H
#define FAN_H


// Fan class
class Fan {

	// Public
	public :
	
		// Constructor
		Fan();
		
		// Turn on
		void turnOn();
		
		// Turn off
		void turnOff();
		
		// Set speed
		void setSpeed(uint8_t speed);
};


#endif
