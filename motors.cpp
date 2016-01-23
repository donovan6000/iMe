// Header files
extern "C" {
	#include <asf.h>
}
#include <math.h>
#include "motors.h"
#include "eeprom.h"


// Definitions

// Motors pins
#define MOTORS_ENABLE IOPORT_CREATE_PIN(PORTB, 3)
#define MOTORS_STEP_CONTROL IOPORT_CREATE_PIN(PORTB, 2)

// Motor X pins
#define MOTOR_X_DIRECTION IOPORT_CREATE_PIN(PORTC, 2)
#define MOTOR_X_VREF IOPORT_CREATE_PIN(PORTD, 1)
#define MOTOR_X_STEP IOPORT_CREATE_PIN(PORTC, 5)
#define MOTOR_X_STEP_PWM_TIMER PWM_TCC1
#define MOTOR_X_STEP_PWM_CHANNEL PWM_CH_B

// Motor Y pins
#define MOTOR_Y_DIRECTION IOPORT_CREATE_PIN(PORTD, 5)
#define MOTOR_Y_VREF IOPORT_CREATE_PIN(PORTD, 3)
#define MOTOR_Y_STEP IOPORT_CREATE_PIN(PORTC, 7)
#define MOTOR_Y_STEP_PWM_TIMER PWM_TCC1
#define MOTOR_Y_STEP_PWM_CHANNEL PWM_CH_D

// Motor Z pins
#define MOTOR_Z_DIRECTION IOPORT_CREATE_PIN(PORTD, 4)
#define MOTOR_Z_VREF IOPORT_CREATE_PIN(PORTD, 2)
#define MOTOR_Z_STEP IOPORT_CREATE_PIN(PORTC, 6)
#define MOTOR_Z_STEP_PWM_TIMER PWM_TCC1
#define MOTOR_Z_STEP_PWM_CHANNEL PWM_CH_C

// Motor E pins
#define MOTOR_E_DIRECTION IOPORT_CREATE_PIN(PORTC, 3)
#define MOTOR_E_VREF IOPORT_CREATE_PIN(PORTD, 0)
#define MOTOR_E_STEP IOPORT_CREATE_PIN(PORTC, 4)
#define MOTOR_E_STEP_PWM_TIMER PWM_TCC1
#define MOTOR_E_STEP_PWM_CHANNEL PWM_CH_A
#define MOTOR_E_AISEN IOPORT_CREATE_PIN(PORTA, 7)


// Supporting function implementation
Motors::Motors() {

	// Set mode
	mode = ABSOLUTE;
	
	// Set current values
	currentX = NAN;
	currentY = NAN;
	currentZ = NAN;
	currentE = NAN;
	currentF = NAN;
	
	// Check if last Z value was recorded
	if(nvm_eeprom_read_byte(EEPROM_SAVED_Z_STATE_OFFSET))
	
		// Set current Z to last recorded Z value
		nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentZ, EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
	// Otherwise
	else
	
		// Set current Z to not a number
		currentZ = NAN;
	
	// Configure motors
	ioport_set_pin_dir(MOTORS_ENABLE, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTORS_ENABLE, IOPORT_PIN_LEVEL_HIGH);
	
	ioport_set_pin_dir(MOTORS_STEP_CONTROL, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTORS_STEP_CONTROL, IOPORT_PIN_LEVEL_LOW);
	
	// Configure motor X Vref, direction, and step
	ioport_set_pin_dir(MOTOR_X_VREF, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_X_VREF, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_X_DIRECTION, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_X_DIRECTION, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_X_STEP, IOPORT_DIR_OUTPUT);
	pwm_config motorXStepPwm;
	pwm_init(&motorXStepPwm, MOTOR_X_STEP_PWM_TIMER, MOTOR_X_STEP_PWM_CHANNEL, 5000);
	pwm_start(&motorXStepPwm, 50);
	
	// Configure motor Y Vref, direction, and step
	ioport_set_pin_dir(MOTOR_Y_VREF, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Y_VREF, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_Y_DIRECTION, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Y_DIRECTION, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_Y_STEP, IOPORT_DIR_OUTPUT);
	pwm_config motorYStepPwm;
	pwm_init(&motorYStepPwm, MOTOR_Y_STEP_PWM_TIMER, MOTOR_Y_STEP_PWM_CHANNEL, 5000);
	pwm_start(&motorYStepPwm, 50);
	
	// Configure motor Z VREF, direction, and step
	ioport_set_pin_dir(MOTOR_Z_VREF, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Z_VREF, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_Z_DIRECTION, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Z_DIRECTION, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_Z_STEP, IOPORT_DIR_OUTPUT);
	pwm_config motorZStepPwm;
	pwm_init(&motorZStepPwm, MOTOR_Z_STEP_PWM_TIMER, MOTOR_Z_STEP_PWM_CHANNEL, 5000);
	pwm_start(&motorZStepPwm, 50);
	
	// Configure motor E VREF, direction, step, and AISEN
	ioport_set_pin_dir(MOTOR_E_VREF, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_E_VREF, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_E_DIRECTION, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_E_DIRECTION, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(MOTOR_E_STEP, IOPORT_DIR_OUTPUT);
	pwm_config motorEStepPwm;
	pwm_init(&motorEStepPwm, MOTOR_E_STEP_PWM_TIMER, MOTOR_E_STEP_PWM_CHANNEL, 5000);
	pwm_start(&motorEStepPwm, 50);
	
	ioport_set_pin_dir(MOTOR_E_AISEN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(MOTOR_E_AISEN, IOPORT_MODE_PULLDOWN);
}

void Motors::turnOn() {

	// Turn on motors
	ioport_set_pin_level(MOTORS_ENABLE, IOPORT_PIN_LEVEL_LOW);
}

void Motors::turnOff() {

	// Turn off motors
	ioport_set_pin_level(MOTORS_ENABLE, IOPORT_PIN_LEVEL_HIGH);
}

void Motors::setMode(MODES mode) {

	// Set mode
	this->mode = mode;
}

void Motors::move(const Gcode &command) {

	// Motor X test
	turnOn();
	ioport_set_pin_level(MOTOR_X_VREF, IOPORT_PIN_LEVEL_HIGH);
}
