// Header gaurd
#ifndef LED_H
#define LED_H


// Definitions
#define LED_MAX_BRIGHTNESS 100


// LED class
class Led {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Set brightness
		void setBrightness(uint8_t brightness);
};


#endif
