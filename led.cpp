// Header files
extern "C" {
	#include <asf.h>
}
#include "led.h"


// Definitions

// LED pin
#define LED_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 3)
#define LED_PWM_TIMER PWM_TCE0
#define LED_PWM_CHANNEL PWM_CH_D


// Global variables
pwm_config ledPwm;


// Supporting function implementation
Led::Led() {

	// Configure LED
	ioport_set_pin_dir(LED_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	pwm_init(&ledPwm, LED_PWM_TIMER, LED_PWM_CHANNEL, 500);
	
	// Turn on LED
	turnOn();
}

void Led::turnOn() {

	// Turn on LED
	setBrightness(100);
}

void Led::turnOff() {

	// Turn off LED
	setBrightness(0);
}

void Led::setBrightness(uint8_t brightness) {

	// Set brightness
	pwm_start(&ledPwm, brightness);
}
