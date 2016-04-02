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
#define NUMBER_OF_MOTORS 4

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
uint32_t motorsStepDelay[NUMBER_OF_MOTORS];
uint32_t motorsStepDelayCounter[NUMBER_OF_MOTORS];
uint32_t motorsNumberOfSteps[NUMBER_OF_MOTORS];
uint32_t motorsNumberOfStepsCounter[NUMBER_OF_MOTORS];
uint32_t motorsDelaySkips[NUMBER_OF_MOTORS];
uint32_t motorsDelaySkipsCounter[NUMBER_OF_MOTORS];


// Supporting function implementation
void stepTimerInterrupt(AXES motor) {

	// Set motor step interrupt to a lower priority
	switch(motor) {
	
		case X:
			tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		break;
		
		case Y:
			tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		break;
		
		case Z:
			tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		break;
		
		default:
			tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	}
	
	// Check if time to skip a motor delay
	if(motorsDelaySkips[motor] > 1 && ++motorsDelaySkipsCounter[motor] >= motorsDelaySkips[motor]) {
	
		// Clear motor skip delay counter
		motorsDelaySkipsCounter[motor] = 0;
		
		// Return
		return;
	}
	
	// Check if time to increment motor step
	if(++motorsStepDelayCounter[motor] >= motorsStepDelay[motor]) {
	
		// Clear motor step counter
		motorsStepDelayCounter[motor] = 0;

		// Check if not exceeding number of steps
		if(++motorsNumberOfStepsCounter[motor] <= motorsNumberOfSteps[motor])

			// Set motor step pin
			switch(motor) {
	
				case X:
					ioport_set_pin_level(MOTOR_X_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
				break;
		
				case Y:
					ioport_set_pin_level(MOTOR_Y_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
				break;
		
				case Z:
					ioport_set_pin_level(MOTOR_Z_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
				break;
		
				default:
					ioport_set_pin_level(MOTOR_E_STEP_PIN, IOPORT_PIN_LEVEL_HIGH);
			}
	
		// Otherwise
		else {
			
			// Disable motor step interrupt
			switch(motor) {
	
				case X:
					tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
				break;
		
				case Y:
					tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
				break;
		
				case Z:
					
					// Set motor Z Vref to idle
					tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
					
					tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
				break;
		
				default:
					tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
			}
		}
	}
}

void Motors::initialize() {

	// Set mode
	mode = ABSOLUTE;
	
	// Set current values
	currentValues[X] = NAN;
	currentValues[Y] = NAN;
	currentValues[E] = 0;
	currentValues[F] = 1000;
	
	// Set current Z
	nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
	// Set speed limits
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_X_OFFSET, &motorsSpeedLimit[X], EEPROM_SPEED_LIMIT_X_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Y_OFFSET, &motorsSpeedLimit[Y], EEPROM_SPEED_LIMIT_Y_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Z_OFFSET, &motorsSpeedLimit[Z], EEPROM_SPEED_LIMIT_Z_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_POSITIVE_OFFSET, &motorsSpeedLimit[E_POSITIVE], EEPROM_SPEED_LIMIT_E_POSITIVE_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_NEGATIVE_OFFSET, &motorsSpeedLimit[E_NEGATIVE], EEPROM_SPEED_LIMIT_E_NEGATIVE_LENGTH);
	
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
	
		// Run step timer interrupt
		stepTimerInterrupt(X);
	});
	
	// Motor Y step timer callback
	tc_set_ccb_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run step timer interrupt
		stepTimerInterrupt(Y);
	});
	
	// Motor Z step timer callback
	tc_set_ccc_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run step timer interrupt
		stepTimerInterrupt(Z);
	});
	
	// Motor E step timer callback
	tc_set_ccd_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run step timer interrupt
		stepTimerInterrupt(E);
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
		default:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_INPUT);
			ioport_set_pin_mode(MOTORS_STEP_CONTROL_PIN, IOPORT_MODE_TOTEM);
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

	// Check if command has an F parameter
	if(command.hasParameterF())
	
		// Set current F
		currentValues[F] = command.getParameterF();
	
	// Initialize variables
	float slowestTime = 0;
	float totalSteps[NUMBER_OF_MOTORS] = {};
	
	// Go through all motors
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
	
		// Set has parameter and get parameter function
		bool (Gcode::*hasParameter)() const;
		float (Gcode::*getParameter)() const;
		switch(i) {
		
			case X:
				hasParameter = &Gcode::hasParameterX;
				getParameter = &Gcode::getParameterX;
			break;
			
			case Y:
				hasParameter = &Gcode::hasParameterY;
				getParameter = &Gcode::getParameterY;
			break;
			
			case Z:
				hasParameter = &Gcode::hasParameterZ;
				getParameter = &Gcode::getParameterZ;
			break;
			
			default:
				hasParameter = &Gcode::hasParameterE;
				getParameter = &Gcode::getParameterE;
		}
	
		// Check if command has parameter
		if((command.*hasParameter)()) {
	
			// Set new value
			float newValue;
			if(mode == RELATIVE && !isnan(currentValues[i]))
				newValue = currentValues[i] + (command.*getParameter)();
			else
				newValue = (command.*getParameter)();
		
			// Check if motor moves
			float distanceTraveled = fabs(newValue - (!isnan(currentValues[i]) ? currentValues[i] : 0));
			if(distanceTraveled) {
			
				// Set lower new value
				bool lowerNewValue = (isnan(currentValues[i]) && newValue < 0) || (!isnan(currentValues[i]) && newValue < currentValues[i]);
				
				// Set current value
				if(!isnan(currentValues[i]))
					currentValues[i] = newValue;
		
				// Set motor direction, steps per mm, speed limit, and min/max feed rates
				float stepsPerMm;
				float speedLimit;
				float maxFeedRate;
				float minFeedRate;
				switch(i) {
				
					case X:
						ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, lowerNewValue ? DIRECTION_LEFT : DIRECTION_RIGHT);
						stepsPerMm = MOTOR_X_STEPS_PER_MM;
						speedLimit = motorsSpeedLimit[X];
						maxFeedRate = MOTOR_X_MAX_FEEDRATE;
						minFeedRate = MOTOR_X_MIN_FEEDRATE;
					break;
					
					case Y:
						ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, lowerNewValue ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
						stepsPerMm = MOTOR_Y_STEPS_PER_MM;
						speedLimit = motorsSpeedLimit[Y];
						maxFeedRate = MOTOR_Y_MAX_FEEDRATE;
						minFeedRate = MOTOR_Y_MIN_FEEDRATE;
					break;
					
					case Z:
						ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, lowerNewValue ? DIRECTION_DOWN : DIRECTION_UP);
						stepsPerMm = MOTOR_Z_STEPS_PER_MM;
						speedLimit = motorsSpeedLimit[Z];
						maxFeedRate = MOTOR_Z_MAX_FEEDRATE;
						minFeedRate = MOTOR_Z_MIN_FEEDRATE;
					break;
					
					default:
						ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, lowerNewValue ? DIRECTION_RETRACT : DIRECTION_EXTRUDE);
						stepsPerMm = MOTOR_E_STEPS_PER_MM;
						if(lowerNewValue) {
							speedLimit = motorsSpeedLimit[E_NEGATIVE];
							maxFeedRate = MOTOR_E_MAX_FEEDRATE_RETRACTION;
						}
						else {
							speedLimit = motorsSpeedLimit[E_POSITIVE];
							maxFeedRate = MOTOR_E_MAX_FEEDRATE_EXTRUSION;
						}
						minFeedRate = MOTOR_E_MIN_FEEDRATE;
				}
				
				// Set total steps
				totalSteps[i] = distanceTraveled * stepsPerMm * step;
				
				// Set motor feedrate
				float motorFeedRate = currentValues[F] > speedLimit ? speedLimit : currentValues[F];
				
				// Enforce min/max feed rates
				if(motorFeedRate > maxFeedRate)
					motorFeedRate = maxFeedRate;
				else if(motorFeedRate < minFeedRate)
					motorFeedRate = minFeedRate;
		
				// Set motor total time
				float motorTotalTime = distanceTraveled / motorFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD;
		
				// Set slowest time
				if(motorTotalTime > slowestTime)
					slowestTime = motorTotalTime;
			}
		}
	}
	
	// Initialize variables
	uint32_t motorsTotalRoundedTime[NUMBER_OF_MOTORS] = {};
	uint32_t slowestRoundedTime = 0;
	bool validZ = false;
	
	// Go through all motors
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++)
	
		// Check if motor moves
		if(totalSteps[i]) {
		
			// Set motor number of steps
			motorsNumberOfStepsCounter[i] = 0;
			motorsNumberOfSteps[i] = round(totalSteps[i]);
		
			// Set motor step delay
			motorsStepDelayCounter[i] = 0;
			motorsStepDelay[i] = round(slowestTime / totalSteps[i]);
		
			// Set motor total rounded time
			motorsTotalRoundedTime[i] = motorsNumberOfSteps[i] * (motorsStepDelay[i] ? motorsStepDelay[i] : 1);
		
			// Set slowest rounded time
			if(motorsTotalRoundedTime[i] > slowestRoundedTime)
				slowestRoundedTime = motorsTotalRoundedTime[i];
		
			// Enable motor step interrupt
			switch(i) {
	
				case X:
					tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
				break;
		
				case Y:
					tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
				break;
		
				case Z:
					tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
					
					// Check if Z is valid
					if((validZ = nvm_eeprom_read_byte(EEPROM_SAVED_Z_STATE_OFFSET)))
		
						// Save that Z is invalid
						nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, INVALID);
		
					// Set motor Z Vref to active
					tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, MOTOR_Z_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD);
				break;
		
				default:
					tc_set_ccd_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
			}
		}
	
	// Go through all motors
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
	
		// Set motor delay skips
		motorsDelaySkipsCounter[i] = 0;
		motorsDelaySkips[i] = slowestRoundedTime != motorsTotalRoundedTime[i] ? round(static_cast<float>(motorsTotalRoundedTime[i]) / (slowestRoundedTime - motorsTotalRoundedTime[i])) : 0;
	}
	
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
	
	// Check if Z motor moved
	if(totalSteps[Z]) {
	
		// Save current Z
		nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
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
	currentValues[X] = 54;
	currentValues[Y] = 50;
	
	// Restore mode
	mode = savedMode;
}

void Motors::setZToZero() {

	// Set current Z
	currentValues[Z] = 0.0999;

	// Save current Z
	nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
	// Save that Z is valid
	nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, VALID);
}

void Motors::emergencyStop() {

	// Disable all motor step interrupts
	MOTORS_STEP_TIMER.INTCTRLB &= ~(TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm);
	
	// Disable motors step timer overflow interrupt
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
}
