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

// Motors settings
#define MOTORS_ENABLE_PIN IOPORT_CREATE_PIN(PORTB, 3)
#define MOTORS_STEP_CONTROL_PIN IOPORT_CREATE_PIN(PORTB, 2)
#define MOTORS_STEP_TIMER TCC0
#define MOTORS_STEP_TIMER_PERIOD 0x400

// Motor X settings
#define MOTOR_X_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 2)
#define MOTOR_X_VREF_PIN IOPORT_CREATE_PIN(PORTD, 1)
#define MOTOR_X_STEP_PIN IOPORT_CREATE_PIN(PORTC, 5)
#define MOTOR_X_VREF_CHANNEL TC_CCB
#define MOTOR_X_VREF_VOLTAGE 0.34600939
#define MOTOR_X_STEPS_PER_MM 19.09165
#define MOTOR_X_MAX_FEEDRATE 4800
#define MOTOR_X_MIN_FEEDRATE 120

// Motor Y settings
#define MOTOR_Y_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 5)
#define MOTOR_Y_VREF_PIN IOPORT_CREATE_PIN(PORTD, 3)
#define MOTOR_Y_STEP_PIN IOPORT_CREATE_PIN(PORTC, 7)
#define MOTOR_Y_VREF_CHANNEL TC_CCD
#define MOTOR_Y_VREF_VOLTAGE 0.34600939
#define MOTOR_Y_STEPS_PER_MM 18.2804
#define MOTOR_Y_MAX_FEEDRATE 4800
#define MOTOR_Y_MIN_FEEDRATE 120

// Motor Z settings
#define MOTOR_Z_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 4)
#define MOTOR_Z_VREF_PIN IOPORT_CREATE_PIN(PORTD, 2)
#define MOTOR_Z_STEP_PIN IOPORT_CREATE_PIN(PORTC, 6)
#define MOTOR_Z_VREF_CHANNEL TC_CCC
#define MOTOR_Z_VREF_VOLTAGE_IDLE 0.098122066
#define MOTOR_Z_VREF_VOLTAGE_ACTIVE 0.299530516
#define MOTOR_Z_STEPS_PER_MM 647.6494
#define MOTOR_Z_MAX_FEEDRATE 60
#define MOTOR_Z_MIN_FEEDRATE 30

// Motor E settings
#define MOTOR_E_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 3)
#define MOTOR_E_VREF_PIN IOPORT_CREATE_PIN(PORTD, 0)
#define MOTOR_E_STEP_PIN IOPORT_CREATE_PIN(PORTC, 4)
#define MOTOR_E_CURRENT_SENSE_PIN IOPORT_CREATE_PIN(PORTA, 7)
#define MOTOR_E_VREF_CHANNEL TC_CCA
#define MOTOR_E_VREF_VOLTAGE 0.149765258
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

// Z states
#define INVALID 0x00
#define VALID 0x01


// Global variables
uint32_t motorXStepDelay;
uint32_t motorXStepCounter;
uint32_t motorXNumberOfStepsCurrent;
uint32_t motorXNumberOfStepsTotal;
uint32_t motorXDelaySkips;
uint32_t motorXDelaySkipsCounter;

uint32_t motorYStepDelay;
uint32_t motorYStepCounter;
uint32_t motorYNumberOfStepsCurrent;
uint32_t motorYNumberOfStepsTotal;
uint32_t motorYDelaySkips;
uint32_t motorYDelaySkipsCounter;

uint32_t motorZStepDelay;
uint32_t motorZStepCounter;
uint32_t motorZNumberOfStepsCurrent;
uint32_t motorZNumberOfStepsTotal;
uint32_t motorZDelaySkips;
uint32_t motorZDelaySkipsCounter;

uint32_t motorEStepDelay;
uint32_t motorEStepCounter;
uint32_t motorENumberOfStepsCurrent;
uint32_t motorENumberOfStepsTotal;
uint32_t motorEDelaySkips;
uint32_t motorEDelaySkipsCounter;


// Supporting function implementation
void Motors::initialize() {

	// Set mode
	mode = ABSOLUTE;
	
	// Set current values
	currentX = NAN;
	currentY = NAN;
	currentE = 0;
	currentF = 1000;
	
	// Set current Z
	nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentZ, EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
	// Set speed limits
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_X_OFFSET, &motorXSpeedLimit, EEPROM_SPEED_LIMIT_X_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Y_OFFSET, &motorYSpeedLimit, EEPROM_SPEED_LIMIT_Y_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Z_OFFSET, &motorZSpeedLimit, EEPROM_SPEED_LIMIT_Z_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_POSITIVE_OFFSET, &motorESpeedLimitExtrude, EEPROM_SPEED_LIMIT_E_POSITIVE_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_NEGATIVE_OFFSET, &motorESpeedLimitRetract, EEPROM_SPEED_LIMIT_E_NEGATIVE_LENGTH);
	
	// Configure motors enable
	ioport_set_pin_dir(MOTORS_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn motors off
	turnOff();
	
	// Set micro steps per step
	setMicroStepsPerStep(STEP32);
	
	// Configure motors Vref timer
	tc_enable(&MOTORS_VREF_TIMER);
	tc_set_wgm(&MOTORS_VREF_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_VREF_TIMER, MOTORS_VREF_TIMER_PERIOD);
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, MOTOR_X_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, MOTOR_Y_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, MOTOR_E_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
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
	tc_set_wgm(&MOTORS_STEP_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD);
	
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
		
		// Check if time to skip a motor X delay
		if(motorXDelaySkips > 1 && ++motorXDelaySkipsCounter >= motorXDelaySkips) {
		
			// Clear motor X skip delay counter
			motorXDelaySkipsCounter = 0;
			
			// Return
			return;
		}
		
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
		
		// Check if time to skip a motor Y delay
		if(motorYDelaySkips > 1 && ++motorYDelaySkipsCounter >= motorYDelaySkips) {
		
			// Clear motor Y skip delay counter
			motorYDelaySkipsCounter = 0;
			
			// Return
			return;
		}
		
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
		
		// Check if time to skip a motor Z delay
		if(motorZDelaySkips > 1 && ++motorZDelaySkipsCounter >= motorZDelaySkips) {
		
			// Clear motor Z skip delay counter
			motorZDelaySkipsCounter = 0;
			
			// Return
			return;
		}
		
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
				tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
				
				// Disable motor Z step interrupt
				tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
			}
		}
	});
	
	// Motor E step timer callback
	tc_set_ccd_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Set motor E step interrupt to a lower priority
		tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		
		// Check if time to skip a motor E delay
		if(motorEDelaySkips > 1 && ++motorEDelaySkipsCounter >= motorEDelaySkips) {
		
			// Clear motor E skip delay counter
			motorEDelaySkipsCounter = 0;
			
			// Return
			return;
		}
		
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

void Motors::move(const Gcode &command) {

	// Initialize variables
	float slowestTime = 0;
	uint32_t slowestRoundedTime = 0;
	uint32_t motorXTotalRoundedTime = 0;
	uint32_t motorYTotalRoundedTime = 0;
	uint32_t motorZTotalRoundedTime = 0;
	uint32_t motorETotalRoundedTime = 0;
	float distanceMmX = 0;
	float distanceMmY = 0;
	float distanceMmZ = 0;
	float distanceMmE = 0;
	bool validZ;

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
		
		// Check if X moves
		if((distanceMmX = fabs(newX - (!isnan(currentX) ? currentX : 0)))) {
		
			// Set motor X direction
			ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, (isnan(currentX) && newX < 0) || (!isnan(currentX) && newX < currentX) ? DIRECTION_LEFT : DIRECTION_RIGHT);
		
			// Set motor X feedrate
			float motorXFeedRate = currentF > motorXSpeedLimit ? motorXSpeedLimit : currentF;
			
			// Enforce min and max feedrate
			if(motorXFeedRate > MOTOR_X_MAX_FEEDRATE)
				motorXFeedRate = MOTOR_X_MAX_FEEDRATE;
			else if(motorXFeedRate < MOTOR_X_MIN_FEEDRATE)
				motorXFeedRate = MOTOR_X_MIN_FEEDRATE;
		
			// Set motor X total time
			float motorXTotalTime = distanceMmX / motorXFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD;
		
			// Set slowest time
			if(motorXTotalTime > slowestTime)
				slowestTime = motorXTotalTime;
		
			// Set current X
			if(!isnan(currentX))
				currentX = newX;
		}
	}
	
	// Check if command has a Y parameter
	if(command.hasParameterY()) {
	
		// Set current Y
		float newY;
		if(mode == RELATIVE && !isnan(currentY))
			newY = currentY + command.getParameterY();
		else
			newY = command.getParameterY();
		
		// Check if Y moved
		if((distanceMmY = fabs(newY - (!isnan(currentY) ? currentY : 0)))) {
		
			// Set motor Y direction
			ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, (isnan(currentY) && newY < 0) || (!isnan(currentY) && newY < currentY) ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
		
			// Set motor Y feedrate
			float motorYFeedRate = currentF > motorYSpeedLimit ? motorYSpeedLimit : currentF;
			
			// Enforce min and max feedrate
			if(motorYFeedRate > MOTOR_Y_MAX_FEEDRATE)
				motorYFeedRate = MOTOR_Y_MAX_FEEDRATE;
			else if(motorYFeedRate < MOTOR_Y_MIN_FEEDRATE)
				motorYFeedRate = MOTOR_Y_MIN_FEEDRATE;
		
			// Set motor Y total time
			float motorYTotalTime = distanceMmY / motorYFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD;
		
			// Set slowest time
			if(motorYTotalTime > slowestTime)
				slowestTime = motorYTotalTime;
		
			// Set current Y
			if(!isnan(currentY))
				currentY = newY;
		}
	}
	
	// Check if command has a Z parameter
	if(command.hasParameterZ()) {
	
		// Set current Z
		float newZ;
		if(mode == RELATIVE)
			newZ = currentZ + command.getParameterZ();
		else
			newZ = command.getParameterZ();
		
		// Check if Z moves
		if((distanceMmZ = fabs(newZ - currentZ))) {
		
			// Set motor Z direction
			ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, newZ < currentZ ? DIRECTION_DOWN : DIRECTION_UP);
		
			// Set motor Z feedrate
			float motorZFeedRate = currentF > motorZSpeedLimit ? motorZSpeedLimit : currentF;
			
			// Enforce min and max feedrate
			if(motorZFeedRate > MOTOR_Z_MAX_FEEDRATE)
				motorZFeedRate = MOTOR_Z_MAX_FEEDRATE;
			else if(motorZFeedRate < MOTOR_Z_MIN_FEEDRATE)
				motorZFeedRate = MOTOR_Z_MIN_FEEDRATE;
		
			// Set motor Z total time
			float motorZTotalTime = distanceMmZ / motorZFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD;
		
			// Set slowest time
			if(motorZTotalTime > slowestTime)
				slowestTime = motorZTotalTime;
		
			// Set current Z
			currentZ = newZ;
		}
	}
	
	// Check if command has a E parameter
	if(command.hasParameterE()) {
	
		// Set new E
		float newE;
		if(mode == RELATIVE)
			newE = currentE + command.getParameterE();
		else
			newE = command.getParameterE();
		
		// Check if E moves
		if((distanceMmE = fabs(newE - currentE))) {
		
			// Set motor E direction
			ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, newE < currentE ? DIRECTION_RETRACT : DIRECTION_EXTRUDE);
		
			// Set motor E feedrate
			float motorEFeedRate = currentF;
			if(ioport_get_pin_level(MOTOR_E_DIRECTION_PIN) == DIRECTION_EXTRUDE && motorEFeedRate > motorESpeedLimitExtrude)
				motorEFeedRate = motorESpeedLimitExtrude;
			else if(ioport_get_pin_level(MOTOR_E_DIRECTION_PIN) == DIRECTION_RETRACT && motorEFeedRate > motorESpeedLimitExtrude)
				motorEFeedRate = motorESpeedLimitRetract;
			
			// Enforce min and max feedrate
			if(ioport_get_pin_level(MOTOR_E_DIRECTION_PIN) == DIRECTION_EXTRUDE && motorEFeedRate > MOTOR_E_MAX_FEEDRATE_EXTRUSION)
				motorEFeedRate = MOTOR_E_MAX_FEEDRATE_EXTRUSION;
			else if(ioport_get_pin_level(MOTOR_E_DIRECTION_PIN) == DIRECTION_RETRACT && motorEFeedRate > MOTOR_E_MAX_FEEDRATE_RETRACTION)
				motorEFeedRate = MOTOR_E_MAX_FEEDRATE_RETRACTION;
			else if(motorEFeedRate < MOTOR_E_MIN_FEEDRATE)
				motorEFeedRate = MOTOR_E_MIN_FEEDRATE;
		
			// Set motor E total time
			float motorETotalTime = distanceMmE / motorEFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD;
		
			// Set slowest time
			if(motorETotalTime > slowestTime)
				slowestTime = motorETotalTime;
		
			// Set current E
			currentE = newE;
		}
	}
	
	// Check if X moves
	if(distanceMmX) {
	
		// Set total X steps
		float totalXSteps = distanceMmX * MOTOR_X_STEPS_PER_MM * step;
		
		// Set motor X number of steps total
		motorXNumberOfStepsCurrent = 0;
		motorXNumberOfStepsTotal = round(totalXSteps);
		
		// Set motor X step delay
		motorXStepCounter = 0;
		motorXStepDelay = round(slowestTime / totalXSteps);
		
		// Set motor X total rounded time
		motorXTotalRoundedTime = motorXNumberOfStepsTotal * (motorXStepDelay ? motorXStepDelay : 1);
		
		// Set slowest rounded time
		if(motorXTotalRoundedTime > slowestRoundedTime)
			slowestRoundedTime = motorXTotalRoundedTime;
		
		// Enable motor X step interrupt
		tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Check if Y moves
	if(distanceMmY) {
	
		// Set total Y steps
		float totalYSteps = distanceMmY * MOTOR_Y_STEPS_PER_MM * step;
	
		// Set motor Y number of steps total
		motorYNumberOfStepsCurrent = 0;
		motorYNumberOfStepsTotal = round(totalYSteps);
		
		// Set motor Y step delay
		motorYStepCounter = 0;
		motorYStepDelay = round(slowestTime / totalYSteps);
		
		// Set motor Y total rounded time
		motorYTotalRoundedTime = motorYNumberOfStepsTotal * (motorYStepDelay ? motorYStepDelay : 1);
		
		// Set slowest rounded time
		if(motorYTotalRoundedTime > slowestRoundedTime)
			slowestRoundedTime = motorYTotalRoundedTime;
		
		// Enable motor Y step interrupt
		tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Check if Z moves
	if(distanceMmZ) {
	
		// Set total Z steps
		float totalZSteps = distanceMmZ * MOTOR_Z_STEPS_PER_MM * step;
		
		// Set motor Z number of steps total
		motorZNumberOfStepsCurrent = 0;
		motorZNumberOfStepsTotal = round(totalZSteps);
		
		// Set motor Z step delay
		motorZStepCounter = 0;
		motorZStepDelay = round(slowestTime / totalZSteps);
		
		// Set motor Z total rounded time
		motorZTotalRoundedTime = motorZNumberOfStepsTotal * (motorZStepDelay ? motorZStepDelay : 1);
		
		// Set slowest rounded time
		if(motorZTotalRoundedTime > slowestRoundedTime)
			slowestRoundedTime = motorZTotalRoundedTime;
		
		// Enable motor Z step interrupt
		tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		
		// Check if Z is valid
		if((validZ = nvm_eeprom_read_byte(EEPROM_SAVED_Z_STATE_OFFSET)))
		
			// Save that Z is invalid
			nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, INVALID);
		
		// Set motor Z Vref to active
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
	}
	
	// Check if E moves
	if(distanceMmE) {
	
		// Set total E steps
		float totalESteps = distanceMmE * MOTOR_E_STEPS_PER_MM * step;
		
		// Set motor E number of steps total
		motorENumberOfStepsCurrent = 0;
		motorENumberOfStepsTotal = round(totalESteps);
		
		// Set motor E step delay
		motorEStepCounter = 0;
		motorEStepDelay = round(slowestTime / totalESteps);
		
		// Set motor E total rounded time
		motorETotalRoundedTime = motorENumberOfStepsTotal * (motorEStepDelay ? motorEStepDelay : 1);
		
		// Set slowest rounded time
		if(motorETotalRoundedTime > slowestRoundedTime)
			slowestRoundedTime = motorETotalRoundedTime;
		
		// Enable motor E step interrupt
		tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Set motor X delay skips
	motorXDelaySkipsCounter = 0;
	motorXDelaySkips = slowestRoundedTime != motorXTotalRoundedTime ? round(static_cast<float>(motorXTotalRoundedTime) / (slowestRoundedTime - motorXTotalRoundedTime)) : 0;
	
	// Set motor Y delay skips
	motorYDelaySkipsCounter = 0;
	motorYDelaySkips = slowestRoundedTime != motorYTotalRoundedTime ? round(static_cast<float>(motorYTotalRoundedTime) / (slowestRoundedTime - motorYTotalRoundedTime)) : 0;
	
	// Set motor Z delay skips
	motorZDelaySkipsCounter = 0;
	motorZDelaySkips = slowestRoundedTime != motorZTotalRoundedTime ? round(static_cast<float>(motorZTotalRoundedTime) / (slowestRoundedTime - motorZTotalRoundedTime)) : 0;
	
	// Set motor E delay skips
	motorEDelaySkipsCounter = 0;
	motorEDelaySkips = slowestRoundedTime != motorETotalRoundedTime ? round(static_cast<float>(motorETotalRoundedTime) / (slowestRoundedTime - motorETotalRoundedTime)) : 0;
	
	// Turn on motors
	turnOn();
	
	// Start motors step timer
	tc_write_count(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD - 1);
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_MED);
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Wait until all motors step interrupts have stopped
	while(MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm));
	
	// Stop motors step timer
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
	
	// Check if Z moved
	if(distanceMmZ) {
	
		// Save current Z
		nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentZ, EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
		// Check if an emergency stop didn't happen and Z was previously valid
		if(~(MOTORS_STEP_TIMER.INTCTRLA & TC0_OVFINTLVL_gm) && validZ)
		
			// Save that Z is valid
			nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, VALID);
	}
	
	// Disable motors step timer overflow interrupt
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
}

void Motors::goHome() {

	// Save mode
	MODES savedMode = mode;
	
	// Move to corner
	mode = RELATIVE;
	Gcode gcode(const_cast<char *>("G0 X109 Y108 F4800"));
	move(gcode);
	
	// Move to center
	gcode.parseCommand(const_cast<char *>("G0 X-54 Y-50 F4800"));
	move(gcode);
	
	// Set current X and Y
	currentX = 54;
	currentY = 50;
	
	// Restore mode
	mode = savedMode;
}

void Motors::setZToZero() {

	// Set current Z
	currentZ = 0.0999;

	// Save current Z
	nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentZ, EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
	// Save that Z is valid
	nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, VALID);
}

void Motors::emergencyStop() {

	// Disable all motor step interrupts
	tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
	
	// Disable motors step timer overflow interrupt
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
}
