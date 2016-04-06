// Header gaurd
#ifndef FAN_H
#define FAN_H


// Definitions
#define FAN_TIMER TCE0
#define FAN_TIMER_PERIOD 0x208D


// Fan class
class Fan {

	// Public
	public :
	
		// Constructor
		Fan();
		
		// Set speed
		void setSpeed(uint8_t speed);
};


#endif
