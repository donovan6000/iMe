// Header files
extern "C" {
	#include <asf.h>
}
#include "led.h"
#include "fan.h"


// Definitions
#define LED_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 3)
#define LED_CHANNEL TC_CCD
#define LED_TIMER FAN_TIMER
#define LED_TIMER_PERIOD FAN_TIMER_PERIOD


// Supporting function implementation
void Led::initialize() {

	// Configure LED
	ioport_set_pin_dir(LED_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn on LED
	setBrightness(100);
}

void Led::setBrightness(uint8_t brightness) {

	// Set brightness
	tc_write_cc(&LED_TIMER, LED_CHANNEL, static_cast<float>(brightness) * LED_TIMER_PERIOD / 100);
}
