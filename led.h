// Header gaurd
#ifndef LED_H
#define LED_H


// Led class
class Led {

	// Public
	public :
	
		// Constructor
		Led();
		
		// Turn on
		void turnOn();
		
		// Turn off
		void turnOff();
		
		// Set brightness
		void setBrightness(uint8_t brightness);
};


#endif
