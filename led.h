// Header guard
#ifndef LED_H
#define LED_H


// Definitions
#define LED_MIN_BRIGHTNESS 0
#define LED_MAX_BRIGHTNESS 100
#define LED_CHANNEL_CAPTURE_COMPARE TC_CCDEN


// LED class
class Led final {

	// Public
	public:
	
		// Initialize
		static void initialize() noexcept;
		
		// Set brightness
		static void setBrightness(uint8_t brightness) noexcept;
		
		// Reset
		static void reset() noexcept;
		
		// Is on
		static bool isOn() noexcept;
};


#endif
