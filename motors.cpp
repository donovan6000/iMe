// DRV8834 http://www.ti.com/lit/ds/slvsb19d/slvsb19d.pdf
// Header files
extern "C" {
	#include <asf.h>
}
#include <math.h>
#include <string.h>
#include "motors.h"
#include "eeprom.h"


// Definitions
#define MICROCONTROLLER_VOLTAGE 3.3

// Motors pins
#define MOTORS_ENABLE_PIN IOPORT_CREATE_PIN(PORTB, 3)
#define MOTORS_STEP_CONTROL_PIN IOPORT_CREATE_PIN(PORTB, 2)
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_STEP_TIMER TCC0

// Motor X pins
#define MOTOR_X_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 2)
#define MOTOR_X_VREF_PIN IOPORT_CREATE_PIN(PORTD, 1)
#define MOTOR_X_STEP_PIN IOPORT_CREATE_PIN(PORTC, 5)
#define MOTOR_X_VREF_CHANNEL TC_CCB
#define MOTOR_X_VREF_VOLTAGE 0.33
#define MOTOR_X_STEPS_PER_MM 19.09165
#define MOTOR_X_MAX_FEEDRATE 4800
#define MOTOR_X_MIN_FEEDRATE 120

// Motor Y pins
#define MOTOR_Y_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 5)
#define MOTOR_Y_VREF_PIN IOPORT_CREATE_PIN(PORTD, 3)
#define MOTOR_Y_STEP_PIN IOPORT_CREATE_PIN(PORTC, 7)
#define MOTOR_Y_VREF_CHANNEL TC_CCD
#define MOTOR_Y_VREF_VOLTAGE 0.33
#define MOTOR_Y_STEPS_PER_MM 18.2804
#define MOTOR_Y_MAX_FEEDRATE 4800
#define MOTOR_Y_MIN_FEEDRATE 120

// Motor Z pins
#define MOTOR_Z_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 4)
#define MOTOR_Z_VREF_PIN IOPORT_CREATE_PIN(PORTD, 2)
#define MOTOR_Z_STEP_PIN IOPORT_CREATE_PIN(PORTC, 6)
#define MOTOR_Z_VREF_CHANNEL TC_CCC
#define MOTOR_Z_VREF_VOLTAGE_IDLE 0.09
#define MOTOR_Z_VREF_VOLTAGE_ACTIVE 0.29
#define MOTOR_Z_STEPS_PER_MM 647.6494
#define MOTOR_Z_MAX_FEEDRATE 60
#define MOTOR_Z_MIN_FEEDRATE 30

// Motor E pins
#define MOTOR_E_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 3)
#define MOTOR_E_VREF_PIN IOPORT_CREATE_PIN(PORTD, 0)
#define MOTOR_E_STEP_PIN IOPORT_CREATE_PIN(PORTC, 4)
#define MOTOR_E_CURRENT_SENSE_PIN IOPORT_CREATE_PIN(PORTA, 7)
#define MOTOR_E_VREF_CHANNEL TC_CCA
#define MOTOR_E_VREF_VOLTAGE 0.14
#define MOTOR_E_STEPS_PER_MM 97.75938
#define MOTOR_E_MAX_FEEDRATE_EXTRUSION 600
#define MOTOR_E_MAX_FEEDRATE_RETRACTION 720
#define MOTOR_E_MIN_FEEDRATE 60

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


// Global variables
uint32_t motorXStepDelay;
uint32_t motorXStepCounter;
uint32_t motorXNumberOfStepsCurrent;
uint32_t motorXNumberOfStepsTotal;
uint32_t motorYStepDelay;
uint32_t motorYStepCounter;
uint32_t motorYNumberOfStepsCurrent;
uint32_t motorYNumberOfStepsTotal;
uint32_t motorZStepDelay;
uint32_t motorZStepCounter;
uint32_t motorZNumberOfStepsCurrent;
uint32_t motorZNumberOfStepsTotal;
uint32_t motorEStepDelay;
uint32_t motorEStepCounter;
uint32_t motorENumberOfStepsCurrent;
uint32_t motorENumberOfStepsTotal;


// Supporting function implementation
Motors::Motors() {

	// Set mode
	mode = ABSOLUTE;
	
	// Set current values
	currentX = NAN;
	currentY = NAN;
	currentZ = NAN;
	currentE = NAN;
	currentF = 1000;
	
	/*// Check if last Z value was recorded
	if(nvm_eeprom_read_byte(EEPROM_SAVED_Z_STATE_OFFSET))
	
		// Set current Z to last recorded Z value
		nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentZ, EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);*/
	
	// Configure motors enable
	ioport_set_pin_dir(MOTORS_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn motors off
	turnOff();
	
	// Set micro steps per step
	setMicroStepsPerStep(STEP32);
	
	// Configure motors Vref timer
	tc_enable(&MOTORS_VREF_TIMER);
	tc_set_8bits_mode(&MOTORS_VREF_TIMER);
	tc_set_wgm(&MOTORS_VREF_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_VREF_TIMER, 0xFF);
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, MOTOR_X_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * tc_read_period(&MOTORS_VREF_TIMER));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, MOTOR_Y_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * tc_read_period(&MOTORS_VREF_TIMER));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * tc_read_period(&MOTORS_VREF_TIMER));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, MOTOR_E_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * tc_read_period(&MOTORS_VREF_TIMER));
	tc_enable_cc_channels(&MOTORS_VREF_TIMER, static_cast<tc_cc_channel_mask_enable_t>(TC_CCAEN | TC_CCBEN | TC_CCCEN | TC_CCDEN));
	tc_write_clock_source(&MOTORS_VREF_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Configure motor X Vref, direction, and step
	ioport_set_pin_dir(MOTOR_X_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_X_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_X_STEP_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure motor Y Vref, direction, and step
	ioport_set_pin_dir(MOTOR_Y_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_Y_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_Y_STEP_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure motor Z VREF, direction, and step
	ioport_set_pin_dir(MOTOR_Z_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_Z_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_Z_STEP_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure motor E VREF, direction, step, and current sense
	ioport_set_pin_dir(MOTOR_E_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_E_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_E_STEP_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_E_CURRENT_SENSE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(MOTOR_E_CURRENT_SENSE_PIN, IOPORT_MODE_PULLDOWN);
	
	// Configure motors step timer
	tc_enable(&MOTORS_STEP_TIMER);
	tc_set_8bits_mode(&MOTORS_STEP_TIMER);
	tc_set_wgm(&MOTORS_STEP_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_STEP_TIMER, sysclk_get_cpu_hz() / 100000);
	tc_write_cc(&MOTORS_STEP_TIMER, TC_CCA, 0);
	tc_write_cc(&MOTORS_STEP_TIMER, TC_CCB, 0);
	tc_write_cc(&MOTORS_STEP_TIMER, TC_CCC, 0);
	tc_write_cc(&MOTORS_STEP_TIMER, TC_CCD, 0);
	tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	
	// Motors step timer overflow callback
	tc_set_overflow_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {

		// Clear motor X, Y, Z, and E step pins
		ioport_set_pin_level(MOTOR_X_STEP_PIN, IOPORT_PIN_LEVEL_LOW);
		ioport_set_pin_level(MOTOR_Y_STEP_PIN, IOPORT_PIN_LEVEL_LOW);
		ioport_set_pin_level(MOTOR_Z_STEP_PIN, IOPORT_PIN_LEVEL_LOW);
		ioport_set_pin_level(MOTOR_E_STEP_PIN, IOPORT_PIN_LEVEL_LOW);
		
		// Set motor X step interrupt to a higher priority if enabled
		if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCAINTLVL_gm)
			tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_HI);
		
		// Set motor Y step interrupt to a higher priority if enabled
		if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCBINTLVL_gm)
			tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_HI);
		
		// Set motor Z step interrupt to a higher priority if enabled
		if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCCINTLVL_gm)
			tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_HI);
		
		// Set motor E step interrupt to a higher priority if enabled
		if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm)
			tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_HI);
	});
	
	// Motor X step timer callback
	tc_set_cca_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Set motor X step interrupt to a lower priority
		tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	
		// Check if time to increment motor X step
		if(++motorXStepCounter >= motorXStepDelay) {
		
			// Clear motor X step counter
			motorXStepCounter = 0;
	
			// Check if not exceeding total number of steps
			if(++motorXNumberOfStepsCurrent <= motorXNumberOfStepsTotal)
	
				// Set motor X step pin
				ioport_set_pin_level(MOTOR_X_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
		
			// Otherwise
			else
		
				// Disable motor X step interrupt
				tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
		}
	});
	
	// Motor Y step timer callback
	tc_set_ccb_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Set motor Y step interrupt to a lower priority
		tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	
		// Check if time to increment motor Y step
		if(++motorYStepCounter >= motorYStepDelay) {
		
			// Clear motor Y step counter
			motorYStepCounter = 0;
	
			// Check if not exceeding total number of steps
			if(++motorYNumberOfStepsCurrent <= motorYNumberOfStepsTotal)
	
				// Set motor Y step pin
				ioport_set_pin_level(MOTOR_Y_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
		
			// Otherwise
			else
		
				// Disable motor Y step interrupt
				tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
		}
	});
	
	// Motor Z step timer callback
	tc_set_ccc_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Set motor Z step interrupt to a lower priority
		tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	
		// Check if time to increment motor Z step
		if(++motorZStepCounter >= motorZStepDelay) {
		
			// Clear motor Z step counter
			motorZStepCounter = 0;
	
			// Check if not exceeding total number of steps
			if(++motorZNumberOfStepsCurrent <= motorZNumberOfStepsTotal)
	
				// Set motor Z step pin
				ioport_set_pin_level(MOTOR_Z_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
		
			// Otherwise
			else {
		
				// Set motor Z Vref to idle
				tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * tc_read_period(&MOTORS_VREF_TIMER));
			
				// Disable motor Z step interrupt
				tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
			}
		}
	});
	
	// Motor E step timer callback
	tc_set_ccd_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Set motor E step interrupt to a lower priority
		tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	
		// Check if time to increment motor E step
		if(++motorEStepCounter >= motorEStepDelay) {
		
			// Clear motor E step counter
			motorEStepCounter = 0;
	
			// Check if not exceeding total number of steps
			if(++motorENumberOfStepsCurrent <= motorENumberOfStepsTotal)
	
				// Set motor E step pin
				ioport_set_pin_level(MOTOR_E_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
		
			// Otherwise
			else
		
				// Disable motor E step interrupt
				tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
		}
	});
}

void Motors::setMicroStepsPerStep(STEPS step) {

	// Set steps
	this->step = step;

	// Check specified micro steps per step
	switch(step) {
	
		// 8 micro steps per step
		case STEP8:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
			ioport_set_pin_level(MOTORS_ENABLE_PIN, IOPORT_PIN_LEVEL_LOW);
		break;
		
		// 16 micro steps per step
		case STEP16:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
			ioport_set_pin_level(MOTORS_ENABLE_PIN, IOPORT_PIN_LEVEL_HIGH);
		break;
		
		// 32 micro steps per step
		case STEP32:
		
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
}

void Motors::setMode(MODES mode) {

	// Set mode
	this->mode = mode;
}

void Motors::move(const Gcode &command) {

	// Initialize variables
	float slowestTime = -INFINITY;
	float distanceMmX = NAN;
	float distanceMmY = NAN;
	float distanceMmZ = NAN;
	float distanceMmE = NAN;

	// Check if command has an F parameter
	if(command.hasParameterF())
	
		// Set current F
		currentF = command.getParameterF();
	
	// Check if command has an X parameter
	if(command.hasParameterX()) {
	
		// Set new X
		float newX;
		if(mode == RELATIVE && !isnan(currentX))
			newX = currentX + command.getParameterX();
		else
			newX = command.getParameterX();
		
		// Set motor X direction
		ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, (isnan(currentX) && newX < 0) || (!isnan(currentX) && newX < currentX) ? DIRECTION_LEFT : DIRECTION_RIGHT);
		
		// Set distance mm X
		distanceMmX = fabs(newX - (!isnan(currentX) ? currentX : 0));
		
		// Set motor X feedrate
		float motorXFeedRate = currentF;
		if(motorXFeedRate > MOTOR_X_MAX_FEEDRATE)
			motorXFeedRate = MOTOR_X_MAX_FEEDRATE;
		else if(motorXFeedRate < MOTOR_X_MIN_FEEDRATE)
			motorXFeedRate = MOTOR_X_MIN_FEEDRATE;
		
		// Set motor X total time
		float motorXTotalTime = distanceMmX / motorXFeedRate * 60 * sysclk_get_cpu_hz() / tc_read_period(&MOTORS_STEP_TIMER);
		
		// Set slowest time
		if(motorXTotalTime > slowestTime)
			slowestTime = motorXTotalTime;
		
		// Set current X
		if(mode != RELATIVE || !isnan(currentX))
			currentX = newX;
	}
	
	// Check if command has a Y parameter
	if(command.hasParameterY()) {
	
		// Set current Y
		float newY;
		if(mode == RELATIVE && !isnan(currentY))
			newY = currentY + command.getParameterY();
		else
			newY = command.getParameterY();
		
		// Set motor Y direction
		ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, (isnan(currentY) && newY < 0) || (!isnan(currentY) && newY < currentY) ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
		
		// Set distance mm Y
		distanceMmY = fabs(newY - (!isnan(currentY) ? currentY : 0));
		
		// Set motor Y feedrate
		float motorYFeedRate = currentF;
		if(motorYFeedRate > MOTOR_Y_MAX_FEEDRATE)
			motorYFeedRate = MOTOR_Y_MAX_FEEDRATE;
		else if(motorYFeedRate < MOTOR_Y_MIN_FEEDRATE)
			motorYFeedRate = MOTOR_Y_MIN_FEEDRATE;
		
		// Set motor Y total time
		float motorYTotalTime = distanceMmY / motorYFeedRate * 60 * sysclk_get_cpu_hz() / tc_read_period(&MOTORS_STEP_TIMER);
		
		// Set slowest time
		if(motorYTotalTime > slowestTime)
			slowestTime = motorYTotalTime;
		
		// Set current Y
		if(mode != RELATIVE || !isnan(currentY))
			currentY = newY;
	}
	
	// Check if command has a Z parameter
	if(command.hasParameterZ()) {
	
		// Set current Z
		float newZ;
		if(mode == RELATIVE && !isnan(currentZ))
			newZ = currentZ + command.getParameterZ();
		else
			newZ = command.getParameterZ();
		
		// Set motor Z direction
		ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, (isnan(currentZ) && newZ < 0) || (!isnan(currentZ) && newZ < currentZ) ? DIRECTION_DOWN : DIRECTION_UP);
		
		// Set distance mm Z
		distanceMmZ = fabs(newZ - (!isnan(currentZ) ? currentZ : 0));
		
		// Set motor Z feedrate
		float motorZFeedRate = currentF;
		if(motorZFeedRate > MOTOR_Z_MAX_FEEDRATE)
			motorZFeedRate = MOTOR_Z_MAX_FEEDRATE;
		else if(motorZFeedRate < MOTOR_Z_MIN_FEEDRATE)
			motorZFeedRate = MOTOR_Z_MIN_FEEDRATE;
		
		// Set motor Z total time
		float motorZTotalTime = distanceMmZ / motorZFeedRate * 60 * sysclk_get_cpu_hz() / tc_read_period(&MOTORS_STEP_TIMER);
		
		// Set slowest time
		if(motorZTotalTime > slowestTime)
			slowestTime = motorZTotalTime;
		
		// Set current Z
		if(mode != RELATIVE || !isnan(currentZ))
			currentZ = newZ;
	}
	
	// Check if command has a E parameter
	if(command.hasParameterE()) {
	
		// Set new E
		float newE;
		if(mode == RELATIVE && !isnan(currentE))
			newE = currentE + command.getParameterE();
		else
			newE = command.getParameterE();
		
		// Set motor E direction
		ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, (isnan(currentE) && newE < 0) || (!isnan(currentE) && newE < currentE) ? DIRECTION_RETRACT : DIRECTION_EXTRUDE);
		
		// Set distance mm E
		distanceMmE = fabs(newE - (!isnan(currentE) ? currentE : 0));
		
		// Set motor E feedrate
		float motorEFeedRate = currentF;
		if(ioport_get_pin_level(MOTOR_E_DIRECTION_PIN) == DIRECTION_RETRACT && motorEFeedRate > MOTOR_E_MAX_FEEDRATE_RETRACTION)
			motorEFeedRate = MOTOR_E_MAX_FEEDRATE_RETRACTION;
		else if(ioport_get_pin_level(MOTOR_E_DIRECTION_PIN) == DIRECTION_EXTRUDE && motorEFeedRate > MOTOR_E_MAX_FEEDRATE_EXTRUSION)
			motorEFeedRate = MOTOR_E_MAX_FEEDRATE_EXTRUSION;
		else if(motorEFeedRate < MOTOR_E_MIN_FEEDRATE)
			motorEFeedRate = MOTOR_E_MIN_FEEDRATE;
		
		// Set motor E total time
		float motorETotalTime = distanceMmE / motorEFeedRate * 60 * sysclk_get_cpu_hz() / tc_read_period(&MOTORS_STEP_TIMER);
		
		// Set slowest time
		if(motorETotalTime > slowestTime)
			slowestTime = motorETotalTime;
		
		// Set current E
		if(mode != RELATIVE || !isnan(currentE))
			currentE = newE;
	}
	
	// Check if using X motor
	if(!isnan(distanceMmX)) {
	
		// Set total X steps
		float totalXSteps = distanceMmX * MOTOR_X_STEPS_PER_MM * step;
	
		// Set motor X number of steps total
		motorXNumberOfStepsCurrent = 0;
		motorXNumberOfStepsTotal = round(totalXSteps);
		
		// Set motor X step delay
		motorXStepCounter = 0;
		motorXStepDelay = round(slowestTime / totalXSteps);
		
		// Enable motor X step interrupt
		tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Check if using Y motor
	if(!isnan(distanceMmY)) {
	
		// Set total Y steps
		float totalYSteps = distanceMmY * MOTOR_Y_STEPS_PER_MM * step;
	
		// Set motor Y number of steps total
		motorYNumberOfStepsCurrent = 0;
		motorYNumberOfStepsTotal = round(totalYSteps);
		
		// Set motor Y step delay
		motorYStepCounter = 0;
		motorYStepDelay = round(slowestTime / totalYSteps);
		
		// Enable motor Y step interrupt
		tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Check if using Z motor
	if(!isnan(distanceMmZ)) {
	
		// Set total Z steps
		float totalZSteps = distanceMmZ * MOTOR_Z_STEPS_PER_MM * step;
	
		// Set motor Z number of steps total
		motorZNumberOfStepsCurrent = 0;
		motorZNumberOfStepsTotal = round(totalZSteps);
		
		// Set motor Z step delay
		motorZStepCounter = 0;
		motorZStepDelay = round(slowestTime / totalZSteps);
		
		// Enable motor Z step interrupt
		tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		
		// Set motor Z Vref to active
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * tc_read_period(&MOTORS_VREF_TIMER));
	}
	
	// Check if using E motor
	if(!isnan(distanceMmE)) {
	
		// Set total E steps
		float totalESteps = distanceMmE * MOTOR_E_STEPS_PER_MM * step;
	
		// Set motor E number of steps total
		motorENumberOfStepsCurrent = 0;
		motorENumberOfStepsTotal = round(totalESteps);
		
		// Set motor E step delay
		motorEStepCounter = 0;
		motorEStepDelay = round(slowestTime / totalESteps);
		
		// Enable motor E step interrupt
		tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Turn on motors
	turnOn();
	
	// Start motors step timer
	tc_write_count(&MOTORS_STEP_TIMER, tc_read_period(&MOTORS_STEP_TIMER));
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_MED);
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Wait until all motors step interrupts have stopped
	while(MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm));
	
	// Stop motors step timer
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
}
