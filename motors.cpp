// DRV8834 http://www.ti.com/lit/ds/slvsb19d/slvsb19d.pdf
// Header files
extern "C" {
	#include <asf.h>
}
#include <math.h>
#include "motors.h"
#include "eeprom.h"


// Definitions
#define MICROCONTROLLER_VOLTAGE 3.3

// Motors pins
#define MOTORS_ENABLE_PIN IOPORT_CREATE_PIN(PORTB, 3)
#define MOTORS_STEP_CONTROL_PIN IOPORT_CREATE_PIN(PORTB, 2)

// Motor X pins
#define MOTOR_X_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 2)
#define MOTOR_X_VREF_PIN IOPORT_CREATE_PIN(PORTD, 1)
#define MOTOR_X_VREF_PWM_TIMER PWM_TCD0
#define MOTOR_X_VREF_PWM_CHANNEL PWM_CH_B
#define MOTOR_X_VREF_VOLTAGE 0.33

// Motor Y pins
#define MOTOR_Y_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 5)
#define MOTOR_Y_VREF_PIN IOPORT_CREATE_PIN(PORTD, 3)
#define MOTOR_Y_VREF_PWM_TIMER PWM_TCD0
#define MOTOR_Y_VREF_PWM_CHANNEL PWM_CH_D
#define MOTOR_Y_VREF_VOLTAGE 0.33

// Motor Z pins
#define MOTOR_Z_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 4)
#define MOTOR_Z_VREF_PIN IOPORT_CREATE_PIN(PORTD, 2)
#define MOTOR_Z_VREF_PWM_TIMER PWM_TCD0
#define MOTOR_Z_VREF_PWM_CHANNEL PWM_CH_C
#define MOTOR_Z_VREF_VOLTAGE 0.09

// Motor E pins
#define MOTOR_E_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 3)
#define MOTOR_E_VREF_PIN IOPORT_CREATE_PIN(PORTD, 0)
#define MOTOR_E_VREF_PWM_TIMER PWM_TCD0
#define MOTOR_E_VREF_PWM_CHANNEL PWM_CH_A
#define MOTOR_E_VREF_VOLTAGE 0.14
#define MOTOR_E_CURRENT_SENSE_PIN IOPORT_CREATE_PIN(PORTA, 7)

// Pin states
#define MOTORS_ON IOPORT_PIN_LEVEL_LOW
#define MOTORS_OFF IOPORT_PIN_LEVEL_HIGH
#define DIRECTION_LEFT IOPORT_PIN_LEVEL_HIGH
#define DIRECTION_RIGHT IOPORT_PIN_LEVEL_LOW
#define DIRECTION_BACKWARD IOPORT_PIN_LEVEL_HIGH
#define DIRECTION_FORWARD IOPORT_PIN_LEVEL_LOW
#define DIRECTION_UP IOPORT_PIN_LEVEL_HIGH
#define DIRECTION_DOWN IOPORT_PIN_LEVEL_LOW
#define DIRECTION_EXTRUDE IOPORT_PIN_LEVEL_LOW
#define DIRECTION_RETRACT IOPORT_PIN_LEVEL_HIGH


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
	
	// Configure motors enable
	ioport_set_pin_dir(MOTORS_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn motors off
	turnOff();
	
	// Set micro steps per step
	setMicroStepsPerStep(STEPS32);
	
	// Configure motor X Vref, direction, and step
	ioport_set_pin_dir(MOTOR_X_VREF_PIN, IOPORT_DIR_OUTPUT);
	pwm_config motorXVrefPwm;
	pwm_init(&motorXVrefPwm, MOTOR_X_VREF_PWM_TIMER, MOTOR_X_VREF_PWM_CHANNEL, 50000);
	pwm_start(&motorXVrefPwm, MOTOR_X_VREF_VOLTAGE * 100 / MICROCONTROLLER_VOLTAGE);
	
	ioport_set_pin_dir(MOTOR_X_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, DIRECTION_RIGHT);
	
	// Configure motor Y Vref, direction, and step
	ioport_set_pin_dir(MOTOR_Y_VREF_PIN, IOPORT_DIR_OUTPUT);
	pwm_config motorYVrefPwm;
	pwm_init(&motorYVrefPwm, MOTOR_Y_VREF_PWM_TIMER, MOTOR_Y_VREF_PWM_CHANNEL, 50000);
	pwm_start(&motorYVrefPwm, MOTOR_Y_VREF_VOLTAGE * 100 / MICROCONTROLLER_VOLTAGE);
	
	ioport_set_pin_dir(MOTOR_Y_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, DIRECTION_BACKWARD);
	
	// Configure motor Z VREF, direction, and step
	ioport_set_pin_dir(MOTOR_Z_VREF_PIN, IOPORT_DIR_OUTPUT);
	pwm_config motorZVrefPwm;
	pwm_init(&motorZVrefPwm, MOTOR_Z_VREF_PWM_TIMER, MOTOR_Z_VREF_PWM_CHANNEL, 50000);
	pwm_start(&motorZVrefPwm, MOTOR_Z_VREF_VOLTAGE * 100 / MICROCONTROLLER_VOLTAGE);
	
	ioport_set_pin_dir(MOTOR_Z_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, DIRECTION_UP);
	
	// Configure motor E VREF, direction, step, and current sense
	ioport_set_pin_dir(MOTOR_E_VREF_PIN, IOPORT_DIR_OUTPUT);
	pwm_config motorEVrefPwm;
	pwm_init(&motorEVrefPwm, MOTOR_E_VREF_PWM_TIMER, MOTOR_E_VREF_PWM_CHANNEL, 50000);
	pwm_start(&motorEVrefPwm, MOTOR_E_VREF_VOLTAGE * 100 / MICROCONTROLLER_VOLTAGE);
	
	ioport_set_pin_dir(MOTOR_E_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, DIRECTION_EXTRUDE);
	
	ioport_set_pin_dir(MOTOR_E_CURRENT_SENSE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(MOTOR_E_CURRENT_SENSE_PIN, IOPORT_MODE_PULLDOWN);
}

void Motors::setMicroStepsPerStep(STEPS steps) {

	// Check specified micro steps per step
	switch(steps) {
	
		// 8 micro steps per step
		case STEPS8:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
			ioport_set_pin_level(MOTORS_ENABLE_PIN, IOPORT_PIN_LEVEL_LOW);
		break;
		
		// 16 micro steps per step
		case STEPS16:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
			ioport_set_pin_level(MOTORS_ENABLE_PIN, IOPORT_PIN_LEVEL_HIGH);
		break;
		
		// 32 micro steps per step
		case STEPS32:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_INPUT);
			ioport_set_pin_mode(MOTORS_STEP_CONTROL_PIN, IOPORT_MODE_TOTEM);
		break;
	}
}

void Motors::turnOn() {

	// Turn on motors
	ioport_set_pin_level(MOTORS_ENABLE_PIN, MOTORS_ON);
}

void Motors::turnOff() {

	// Turn off motors
	ioport_set_pin_level(MOTORS_ENABLE_PIN, MOTORS_OFF);
	pwm_stop(&motorXStepPwm);
	pwm_stop(&motorYStepPwm);
	pwm_stop(&motorEStepPwm);
}

void Motors::setMode(MODES mode) {

	// Set mode
	this->mode = mode;
}

void Motors::move(const Gcode &command) {

	/*// Motor X test
	pwm_stop(&motorXStepPwm);
	pwm_timer_reset(&motorXStepPwm);
	pwm_set_frequency(&motorXStepPwm, 1000);
	pwm_start(&motorXStepPwm, 50);*/
	
	pwm_stop(&motorYStepPwm);
	pwm_timer_reset(&motorYStepPwm);
	pwm_set_frequency(&motorYStepPwm, 1000);
	pwm_start(&motorYStepPwm, 50);
	
	/*// Motor E test
	pwm_stop(&motorEStepPwm);
	pwm_timer_reset(&motorEStepPwm);
	pwm_set_frequency(&motorEStepPwm, 1000);
	pwm_start(&motorEStepPwm, 50);*/
	
	// Turn on motors
	turnOn();
}
