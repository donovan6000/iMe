// Header gaurd
#ifndef LED_H
#define LED_H


// Definitions
#define LED_MIN_BRIGHTNESS 0
#define LED_MAX_BRIGHTNESS 100
#define LED_CHANNEL_CAPTURE_COMPARE TC_CCDEN


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
