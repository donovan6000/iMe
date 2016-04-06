// Header files
extern "C" {
	#include <asf.h>
}
#include "led.h"
#include "fan.h"


// Definitions

// LED configuration
#define LED_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 3)
#define LED_CHANNEL TC_CCD


// Supporting function implementation
Led::Led() {

	// Configure LED
	ioport_set_pin_dir(LED_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn on LED
	setBrightness(100);
}

void Led::setBrightness(uint8_t brightness) {

	// Set brightness
	tc_write_cc(&FAN_TIMER, LED_CHANNEL, static_cast<float>(brightness) * FAN_TIMER_PERIOD / 100);
}
