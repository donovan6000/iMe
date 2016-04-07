// DRV8834 http://www.ti.com/lit/ds/slvsb19d/slvsb19d.pdf
// Header files
extern "C" {
	#include <asf.h>
}
#include <math.h>
#include <string.h>
#include "common.h"
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
#define MOTOR_E_CURRENT_SENSE_ADC_CHANNEL ADC_CH0
#define MOTOR_E_CURRENT_SENSE_ADC_PIN ADCCH_POS_PIN7
#define MOTOR_E_VREF_CHANNEL TC_CCA
#define MOTOR_E_VREF_VOLTAGE 0.149765258
#define MOTOR_E_STEPS_PER_MM 97.75938
#define MOTOR_E_MAX_FEEDRATE_EXTRUSION 600
#define MOTOR_E_MAX_FEEDRATE_RETRACTION 720
#define MOTOR_E_MIN_FEEDRATE 60
#define ADC_VREF_PIN IOPORT_CREATE_PIN(PORTA, 0)
#define ADC_VREF 2.6

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
uint32_t motorsDelaySkips[NUMBER_OF_MOTORS];
uint32_t motorsDelaySkipsCounter[NUMBER_OF_MOTORS];
uint32_t motorsStepDelay[NUMBER_OF_MOTORS];
uint32_t motorsStepDelayCounter[NUMBER_OF_MOTORS];
uint32_t motorsNumberOfSteps[NUMBER_OF_MOTORS];


// Supporting function implementation
void stepTimerInterrupt(AXES motor) {

	// Get set motor step interrupt level and motor step pin
	void (*setMotorStepInterruptLevel)(volatile void *tc, TC_INT_LEVEL_t level);
	ioport_pin_t motorStepPin;
	switch(motor) {
	
		case X:
			setMotorStepInterruptLevel = tc_set_cca_interrupt_level;
			motorStepPin = MOTOR_X_STEP_PIN;
		break;
		
		case Y:
			setMotorStepInterruptLevel = tc_set_ccb_interrupt_level;
			motorStepPin = MOTOR_Y_STEP_PIN;
		break;
		
		case Z:
			setMotorStepInterruptLevel = tc_set_ccc_interrupt_level;
			motorStepPin = MOTOR_Z_STEP_PIN;
		break;
		
		default:
			setMotorStepInterruptLevel = tc_set_ccd_interrupt_level;
			motorStepPin = MOTOR_E_STEP_PIN;
	}
	
	// Set motor step interrupt to a lower priority
	(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	
	// Check if time to skip a motor delay
	if(motorsDelaySkips[motor] > 1 && ++motorsDelaySkipsCounter[motor] >= motorsDelaySkips[motor]) {
	
		// Clear motor skip delay counter
		motorsDelaySkipsCounter[motor] = 0;
		
		// Return
		return;
	}
	
	// Check if time to increment motor step
	if(++motorsStepDelayCounter[motor] >= motorsStepDelay[motor]) {

		// Check if moving another step
		if(motorsNumberOfSteps[motor]--)

			// Set motor step pin
			ioport_set_pin_level(motorStepPin, IOPORT_PIN_LEVEL_HIGH);
	
		// Otherwise
		else {
		
			// Reset number of steps
			motorsNumberOfSteps[motor] = 0;
			
			// Disable motor step interrupt
			(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
		}
		
		// Clear motor step counter
		motorsStepDelayCounter[motor] = 0;
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
	
	// Configure motors enable
	ioport_set_pin_dir(MOTORS_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Turn motors off
	turnOff();
	
	// Set micro steps per step
	setMicroStepsPerStep(STEP32);
	
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
	
	// Configure motors Vref timer
	tc_enable(&MOTORS_VREF_TIMER);
	tc_set_wgm(&MOTORS_VREF_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_VREF_TIMER, MOTORS_VREF_TIMER_PERIOD);
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_enable_cc_channels(&MOTORS_VREF_TIMER, static_cast<tc_cc_channel_mask_enable_t>(TC_CCAEN | TC_CCBEN | TC_CCCEN | TC_CCDEN));
	tc_write_clock_source(&MOTORS_VREF_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Configure motors step timer
	tc_enable(&MOTORS_STEP_TIMER);
	tc_set_wgm(&MOTORS_STEP_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD);
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_MED);
	
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
	
	// Configure ADC Vref pin
	ioport_set_pin_dir(ADC_VREF_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(ADC_VREF_PIN, IOPORT_MODE_PULLDOWN);
	
	// Read ADC controller configuration
	adc_config adc_conf;
	adc_read_configuration(&MOTOR_E_CURRENT_SENSE_ADC, &adc_conf);
	
	// Set ADC parameters to be unsigned, 12bit, Vref refrence, manual trigger, 50kHz frequency
	adc_set_conversion_parameters(&adc_conf, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_AREFA);
	adc_set_conversion_trigger(&adc_conf, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&adc_conf, 200000);
	
	// Write ADC controller configuration
	adc_write_configuration(&MOTOR_E_CURRENT_SENSE_ADC, &adc_conf);
	
	// Read ADC channel configuration
	adcch_read_configuration(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL, &currentSenseAdcChannel);
	
	// Set motor E current sense pin as single ended input
	adcch_set_input(&currentSenseAdcChannel, MOTOR_E_CURRENT_SENSE_ADC_PIN, ADCCH_NEG_NONE, 1);
	
	// Enable ADC controller
	adc_enable(&MOTOR_E_CURRENT_SENSE_ADC);
	
	// Initialize accelerometer
	accelerometer.initialize();
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
			ioport_set_pin_level(MOTORS_STEP_CONTROL_PIN, IOPORT_PIN_LEVEL_LOW);
		break;
		
		// 16 micro steps per step
		case STEP16:
		
			// Configure motor's step control
			ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
			ioport_set_pin_level(MOTORS_STEP_CONTROL_PIN, IOPORT_PIN_LEVEL_HIGH);
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
			float tempValue = isnan(currentValues[i]) ? 0 : currentValues[i];
			if(mode == RELATIVE)
				newValue = tempValue + (command.*getParameter)();
			else
				newValue = (command.*getParameter)();
		
			// Check if motor moves
			float distanceTraveled = fabs(newValue - tempValue);
			if(distanceTraveled) {
			
				// Set lower new value
				bool lowerNewValue = newValue < tempValue;
				
				// Set current value
				if(!isnan(currentValues[i]))
					currentValues[i] = newValue;
		
				// Set steps per mm, motor direction, speed limit, and min/max feed rates
				float stepsPerMm;
				float speedLimit;
				float maxFeedRate;
				float minFeedRate;
				switch(i) {
				
					case X:
						stepsPerMm = MOTOR_X_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, lowerNewValue ? DIRECTION_LEFT : DIRECTION_RIGHT);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_X_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_X_LENGTH);
						maxFeedRate = MOTOR_X_MAX_FEEDRATE;
						minFeedRate = MOTOR_X_MIN_FEEDRATE;
					break;
					
					case Y:
						stepsPerMm = MOTOR_Y_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, lowerNewValue ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Y_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_Y_LENGTH);
						maxFeedRate = MOTOR_Y_MAX_FEEDRATE;
						minFeedRate = MOTOR_Y_MIN_FEEDRATE;
					break;
					
					case Z:
						stepsPerMm = MOTOR_Z_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, lowerNewValue ? DIRECTION_DOWN : DIRECTION_UP);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Z_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_Z_LENGTH);
						maxFeedRate = MOTOR_Z_MAX_FEEDRATE;
						minFeedRate = MOTOR_Z_MIN_FEEDRATE;
					break;
					
					default:
						stepsPerMm = MOTOR_E_STEPS_PER_MM;
						if(lowerNewValue) {
							ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, DIRECTION_RETRACT);
							nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_NEGATIVE_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_E_NEGATIVE_LENGTH);
							maxFeedRate = MOTOR_E_MAX_FEEDRATE_RETRACTION;
						}
						else {
							ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, DIRECTION_EXTRUDE);
							nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_POSITIVE_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_E_POSITIVE_LENGTH);
							maxFeedRate = MOTOR_E_MAX_FEEDRATE_EXTRUSION;
						}
						minFeedRate = MOTOR_E_MIN_FEEDRATE;
				}
				
				// Set total steps
				totalSteps[i] = distanceTraveled * stepsPerMm * step;
				
				// Set motor feedrate
				float motorFeedRate = min(currentValues[F], speedLimit);
				
				// Enforce min/max feed rates
				motorFeedRate = min(motorFeedRate, maxFeedRate);
				motorFeedRate = max(motorFeedRate, minFeedRate);
		
				// Set motor total time
				float motorTotalTime = distanceTraveled / motorFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD;
		
				// Set slowest time
				slowestTime = max(motorTotalTime, slowestTime);
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
			motorsNumberOfSteps[i] = round(totalSteps[i]);
		
			// Set motor step delay
			motorsStepDelayCounter[i] = 0;
			motorsStepDelay[i] = round(slowestTime / totalSteps[i]);
		
			// Set motor total rounded time
			motorsTotalRoundedTime[i] = motorsNumberOfSteps[i] * (motorsStepDelay[i] ? motorsStepDelay[i] : 1);
		
			// Set slowest rounded time
			slowestRoundedTime = max(slowestRoundedTime, motorsTotalRoundedTime[i]);
		
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
					tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
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
	
	// Clear Emergency stop occured
	emergencyStopOccured = false;
	
	// Turn on motors
	turnOn();
	
	// Start motors step timer
	tc_write_count(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD - 1);
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Wait until all motors step interrupts have stopped
	while(MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm)) {
	
		// Check if E motor is moving
		if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm) {
		
			// Read average real motor E voltage
			uint32_t value = 0;
			for(uint8_t i = 0; i < 100; i++) {
				adcch_write_configuration(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL, &currentSenseAdcChannel);
				adc_start_conversion(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
				adc_wait_for_interrupt_flag(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
				value += adc_get_result(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
			}
			value /= 100;
			float realVoltage = ADC_VREF / (pow(2, 12) - 1) * value;
			
			// Get ideal motor E voltage
			float idealVoltage = static_cast<float>(tc_read_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL)) / MOTORS_VREF_TIMER_PERIOD * MICROCONTROLLER_VOLTAGE;
			
			// Adjust motor E Vref to maintain a constant motor current
			tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round((MOTOR_E_VREF_VOLTAGE + idealVoltage - realVoltage) / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		}
	}
	
	// Stop motors step timer
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
	
	// Reset motor E Vref
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_VREF_VOLTAGE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	
	// Set motor Z Vref to idle
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	
	// Check if Z motor moved
	if(totalSteps[Z]) {
	
		// Save current Z
		nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
		// Check if an emergency stop didn't happen and Z was previously valid
		if(!emergencyStopOccured && validZ)
		
			// Save that Z is valid
			nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, VALID);
	}
}

void Motors::goHome() {

	// Set up motors to move into corner
	motorsDelaySkips[X] = motorsDelaySkips[Y] = 0;
	motorsStepDelay[X] = motorsStepDelay[Y] = 2;
	motorsNumberOfSteps[X] = motorsNumberOfSteps[Y] = 0x12000;
	
	ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, DIRECTION_RIGHT);
	ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, DIRECTION_BACKWARD);
	tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
	tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);

	// Clear Emergency stop occured
	emergencyStopOccured = false;
	
	// Turn on motors
	turnOn();
	
	// Start motors step timer
	tc_write_count(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD - 1);
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Wait until all motors step interrupts have stopped
	int16_t lastX, lastY;
	uint8_t counterX = 0, counterY = 0;
	for(bool firstRun = true; MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm); firstRun = false) {
		
		// Get accelerometer values
		accelerometer.readAccelerationValues();
		if(!firstRun) {
		
			// Check if motor X has hit the corner
			if(abs(lastX - accelerometer.xValue) >= 15) {
				if(++counterX >= 2)
				
					// Stop motor X interrupt
					tc_set_cca_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
			}
			else
				counterX = 0;
			
			// Check if motor Y has hit the corner
			if(abs(lastY - accelerometer.yValue) >= 15) {
				if(++counterY >= 2)
				
					// Stop motor Y interrupt
					tc_set_ccb_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
			}
			else
				counterY = 0;
		}
		
		// Save accelerometer values
		lastX = accelerometer.xValue;
		lastY = accelerometer.yValue;
	}
	
	// Stop motors step timer
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
	
	// Check if emergenct stop hasn't occured
	if(!emergencyStopOccured) {
	
		// Save mode
		MODES savedMode = mode;
	
		// Move to center
		mode = RELATIVE;
		Gcode gcode(const_cast<char *>("G0 X-54 Y-50 F3000"));
		move(gcode);
		
		// Restore mode
		mode = savedMode;
		
		// Set current X and Y
		currentValues[X] = 54;
		currentValues[Y] = 50;
	}
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

	// Turn off motors
	turnOff();

	// Disable all motor step interrupts
	MOTORS_STEP_TIMER.INTCTRLB &= ~(TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm);
	
	//  Set Emergency stop occured
	emergencyStopOccured = true;
}
