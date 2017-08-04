// Header files
extern "C" {
	#include <asf.h>
}
#include "common.h"
#include "fan.h"
#include "led.h"


// Definitions
#define LED_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 3)
#define LED_CHANNEL TC_CCD
#define LED_TIMER FAN_TIMER
#define LED_TIMER_PERIOD FAN_TIMER_PERIOD


// Supporting function implementation
void Led::initialize() noexcept {

	// Configure LED
	ioport_set_pin_dir(LED_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Reset
	reset();
}

void Led::setBrightness(uint8_t brightness) noexcept {

	// Set brightness
	tc_write_cc(&LED_TIMER, LED_CHANNEL, getValueInRange(brightness, LED_MIN_BRIGHTNESS, LED_MAX_BRIGHTNESS) * LED_TIMER_PERIOD / LED_MAX_BRIGHTNESS);
}

void Led::reset() noexcept {

	// Set LED to max brightness
	setBrightness(LED_MAX_BRIGHTNESS);
}

bool Led::isOn() noexcept {

	// Return if LED isn't at its default brightness
	return tc_read_cc(&LED_TIMER, LED_CHANNEL) != LED_TIMER_PERIOD;
}
