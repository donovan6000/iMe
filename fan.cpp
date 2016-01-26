// Header files
extern "C" {
	#include <asf.h>
}
#include "fan.h"


// Definitions

// Fan pin
#define FAN_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 1)
#define FAN_PWM_TIMER PWM_TCE0
#define FAN_PWM_CHANNEL PWM_CH_B


// Global variables
pwm_config fanPwm;


// Supporting function implementation
Fan::Fan() {

	// Configure fan
	ioport_set_pin_dir(FAN_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	pwm_init(&fanPwm, FAN_PWM_TIMER, FAN_PWM_CHANNEL, 500);
	
	// Turn off fan
	turnOff();
}

void Fan::turnOn() {

	// Turn on fan
	setSpeed(255);
}

void Fan::turnOff() {

	// Turn off fan
	setSpeed(0);
}

void Fan::setSpeed(uint8_t speed) {

	// Set brightness
	pwm_start(&fanPwm, speed);
}
