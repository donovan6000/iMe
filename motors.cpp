// DRV8834 http://www.ti.com/lit/ds/slvsb19d/slvsb19d.pdf
// Header files
extern "C" {
	#include <asf.h>
}
#include <math.h>
#include "motors.h"
#include "eeprom.h"
#include "heater.h"

#include <string.h>
#include "common.h"


// Definitions
#define MICROCONTROLLER_VOLTAGE 3.3
//#define LEGACY_HOMING
#define SEGMENT_LENGTH 2
#define MICROSTEPS_PER_STEP 32

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
#define MOTOR_X_VREF_VOLTAGE_IDLE 0.34600939
#define MOTOR_X_VREF_VOLTAGE_ACTIVE 0.361502347
#define MOTOR_X_STEPS_PER_MM 19.3067875
#define MOTOR_X_MAX_FEEDRATE 4800
#define MOTOR_X_MIN_FEEDRATE 120

// Motor Y settings
#define MOTOR_Y_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 5)
#define MOTOR_Y_VREF_PIN IOPORT_CREATE_PIN(PORTD, 3)
#define MOTOR_Y_STEP_PIN IOPORT_CREATE_PIN(PORTC, 7)
#define MOTOR_Y_VREF_CHANNEL TC_CCD
#define MOTOR_Y_VREF_VOLTAGE_IDLE 0.34600939
#define MOTOR_Y_VREF_VOLTAGE_ACTIVE 0.41314554
#define MOTOR_Y_STEPS_PER_MM 18.00885
#define MOTOR_Y_MAX_FEEDRATE 4800
#define MOTOR_Y_MIN_FEEDRATE 120

// Motor Z settings
#define MOTOR_Z_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 4)
#define MOTOR_Z_VREF_PIN IOPORT_CREATE_PIN(PORTD, 2)
#define MOTOR_Z_STEP_PIN IOPORT_CREATE_PIN(PORTC, 6)
#define MOTOR_Z_VREF_CHANNEL TC_CCC
#define MOTOR_Z_VREF_VOLTAGE_IDLE 0.098122066
#define MOTOR_Z_VREF_VOLTAGE_ACTIVE 0.325352113
#define MOTOR_Z_STEPS_PER_MM 646.3295
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
#define MOTOR_E_VREF_VOLTAGE_IDLE 0.149765258
#define MOTOR_E_VREF_VOLTAGE_ACTIVE 0.149765258
#define MOTOR_E_VREF_VOLTAGE_INITIAL 0.247887324
#define MOTOR_E_STEPS_PER_MM 128.451375
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

// X, Y, and Z states
#define INVALID 0x00
#define VALID 0x01


// Global variables
uint32_t motorsDelaySkips[NUMBER_OF_MOTORS];
uint32_t motorsDelaySkipsCounter[NUMBER_OF_MOTORS];
uint32_t motorsStepDelay[NUMBER_OF_MOTORS];
uint32_t motorsStepDelayCounter[NUMBER_OF_MOTORS];
uint32_t motorsNumberOfSteps[NUMBER_OF_MOTORS];


// Supporting function implementation
Vector calculatePlaneNormalVector(const Vector &v1, const Vector &v2, const Vector &v3) {

	// Initialize variables
	Vector vector, vector2, vector3;
	vector = v2 - v1;
	vector2 = v3 - v1;
	
	// Return normal vector
	vector3[0] = vector[1] * vector2[2] - vector2[1] * vector[2];
	vector3[1] = vector[2] * vector2[0] - vector2[2] * vector[0];
	vector3[2] = vector[0] * vector2[1] - vector2[0] * vector[1];
	vector3[3] = 0;
	return vector3;
}

Vector generatePlaneEquation(const Vector &v1, const Vector &v2, const Vector &v3) {

	// Initialize variables
	Vector vector, vector2;
	vector2 = calculatePlaneNormalVector(v1, v2, v3);
	
	// Return plane equation
	vector[0] = vector2[0];
	vector[1] = vector2[1];
	vector[2] = vector2[2];
	vector[3] = -(vector[0] * v1[0] + vector[1] * v1[1] + vector[2] * v1[2]);
	return vector;
}

float getZFromXYAndPlane(const Vector &point, const Vector &planeABC) {

	// Return Z
	return (planeABC[0] * point.x + planeABC[1] * point.y + planeABC[3]) / -planeABC[2];
}

float sign(const Vector &p1, const Vector &p2, const Vector &p3) {

	// Return sign
	return (p1.x - p3.x) * (p2.y - p3.y) - (p2.x - p3.x) * (p1.y - p3.y);
}

bool isPointInTriangle(const Vector &pt, const Vector &v1, const Vector &v2, const Vector &v3) {

	// Initialize variables
	Vector vector, vector2, vector3, vector4;
	vector = v1 - v2 + v1 - v3;
	vector.normalize();
	vector2 = v1 + vector * 0.01;
	vector = v2 - v1 + v2 - v3;
	vector.normalize();
	vector3 = v2 + vector * 0.01;
	vector = v3 - v1 + v3 - v2;
	vector.normalize();
	vector4 = v3 + vector * 0.01;
	
	// Return if inside triangle
	bool flag = sign(pt, vector2, vector3) < 0;
	bool flag2 = sign(pt, vector3, vector4) < 0;
	bool flag3 = sign(pt, vector4, vector2) < 0;
	return flag == flag2 && flag2 == flag3;
}

float Motors::getHeightAdjustmentRequired(float x, float y) {

	// Initialize variables
	Vector point;
	point.initialize(x, y);
	
	// Return height adjustment
	if(x <= frontLeftVector.x && y >= backRightVector.y)
		return (getZFromXYAndPlane(point, backPlane) + getZFromXYAndPlane(point, leftPlane)) / 2;
	
	else if(x <= frontLeftVector.x && y <= frontLeftVector.y)
		return (getZFromXYAndPlane(point, frontPlane) + getZFromXYAndPlane(point, leftPlane)) / 2;
	
	else if(x >= frontRightVector.x && y <= frontLeftVector.y)
		return (getZFromXYAndPlane(point, frontPlane) + getZFromXYAndPlane(point, rightPlane)) / 2;
	
	else if(x >= frontRightVector.x && y >= backRightVector.y)
		return (getZFromXYAndPlane(point, backPlane) + getZFromXYAndPlane(point, rightPlane)) / 2;
	
	else if(x <= frontLeftVector.x)
		return getZFromXYAndPlane(point, leftPlane);
	
	else if(x >= frontRightVector.x)
		return getZFromXYAndPlane(point, rightPlane);
	
	else if(y >= backRightVector.y)
		return getZFromXYAndPlane(point, backPlane);
	
	else if(y <= frontLeftVector.y)
		return getZFromXYAndPlane(point, frontPlane);
	
	else if(isPointInTriangle(point, centerVector, frontLeftVector, backLeftVector))
		return getZFromXYAndPlane(point, leftPlane);
	
	else if(isPointInTriangle(point, centerVector, frontRightVector, backRightVector))
		return getZFromXYAndPlane(point, rightPlane);
	
	else if(isPointInTriangle(point, centerVector, backLeftVector, backRightVector))
		return getZFromXYAndPlane(point, backPlane);
	
	else
		return getZFromXYAndPlane(point, frontPlane);
}

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
	nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_X_VALUE_OFFSET, &currentValues[X], EEPROM_LAST_RECORDED_X_VALUE_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_Y_VALUE_OFFSET, &currentValues[Y], EEPROM_LAST_RECORDED_Y_VALUE_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	currentValues[E] = 0;
	currentValues[F] = 1000;
	
	// Set bed height offset
	nvm_eeprom_read_buffer(EEPROM_BED_HEIGHT_OFFSET_OFFSET, &bedHeightOffset, EEPROM_BED_HEIGHT_OFFSET_LENGTH);
	
	// Configure motors enable
	ioport_set_pin_dir(MOTORS_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Reset
	reset();
	
	// Set 8 microsteps per step
	#if MICROSTEPS_PER_STEP == 8 
	
		// Configure motor's step control
		ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
		ioport_set_pin_level(MOTORS_STEP_CONTROL_PIN, IOPORT_PIN_LEVEL_LOW);
	
	// Otherwise set 16 microsteps per step
	#elif MICROSTEPS_PER_STEP == 16
	
		// Configure motor's step control
		ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_OUTPUT);
		ioport_set_pin_level(MOTORS_STEP_CONTROL_PIN, IOPORT_PIN_LEVEL_HIGH);
	
	// Otherwise set 32 microsteps per step
	#else
	
		// Configure motor's step control
		ioport_set_pin_dir(MOTORS_STEP_CONTROL_PIN, IOPORT_DIR_INPUT);
		ioport_set_pin_mode(MOTORS_STEP_CONTROL_PIN, IOPORT_MODE_TOTEM);
	#endif
	
	// Configure motor X Vref, direction, and step
	ioport_set_pin_dir(MOTOR_X_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_X_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, nvm_eeprom_read_byte(EEPROM_LAST_RECORDED_X_DIRECTION_OFFSET));
	ioport_set_pin_dir(MOTOR_X_STEP_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure motor Y Vref, direction, and step
	ioport_set_pin_dir(MOTOR_Y_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_Y_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, nvm_eeprom_read_byte(EEPROM_LAST_RECORDED_Y_DIRECTION_OFFSET));
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
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
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
	
	// Set ADC controller to use unsigned, 12bit, Vref refrence, manual trigger, 200kHz frequency
	adc_read_configuration(&MOTOR_E_CURRENT_SENSE_ADC, &currentSenseAdcController);
	adc_set_conversion_parameters(&currentSenseAdcController, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_AREFA);
	adc_set_conversion_trigger(&currentSenseAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&currentSenseAdcController, 200000);
	
	// Set ADC channel to use motor E current sense pin as single ended input with no gain
	adcch_read_configuration(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL, &currentSenseAdcChannel);
	adcch_set_input(&currentSenseAdcChannel, MOTOR_E_CURRENT_SENSE_ADC_PIN, ADCCH_NEG_NONE, 1);
	
	// Initialize accelerometer
	accelerometer.initialize();
	
	// Set vectors
	backRightVector.initialize(99, 95);
	backLeftVector.initialize(9, 95);
	frontLeftVector.initialize(9, 5);
	frontRightVector.initialize(99, 5);
	centerVector.initialize(54, 50);
	
	// Initialize bed leveling G-code
	bedLevelingGcode.valueG = 0;
	bedLevelingGcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_Z_OFFSET | PARAMETER_E_OFFSET;
}

void Motors::turnOn() {

	// Turn on motors
	ioport_set_pin_level(MOTORS_ENABLE_PIN, MOTORS_ON);
}

void Motors::turnOff() {

	// Turn off motors
	ioport_set_pin_level(MOTORS_ENABLE_PIN, MOTORS_OFF);
}

void Motors::move(const Gcode &gcode, uint8_t tasks) {

	// Check if G-code has an F parameter
	if(gcode.commandParameters & PARAMETER_F_OFFSET)
	
		// Save F value
		currentValues[F] = gcode.valueF;
	
	// Initialize variables
	uint32_t slowestTime = 0;
	uint32_t motorMoves[NUMBER_OF_MOTORS] = {};
	BACKLASH_DIRECTION backlashDirectionX = NONE, backlashDirectionY = NONE;
	bool validValues[3];
	
	// Go through all motors
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
	
		// Check if compensating for bed leveling
		if(tasks & BED_LEVELING_TASK)
		
			// Set motor's start value
			startValues[i] = currentValues[i];
	
		// Get parameter offset and parameter value
		uint16_t parameterOffset;
		float newValue;
		switch(i) {
		
			case X:
				parameterOffset = PARAMETER_X_OFFSET;
				newValue = gcode.valueX;
			break;
			
			case Y:
				parameterOffset = PARAMETER_Y_OFFSET;
				newValue = gcode.valueY;
			break;
			
			case Z:
				parameterOffset = PARAMETER_Z_OFFSET;
				newValue = gcode.valueZ;
			break;
			
			default:
				parameterOffset = PARAMETER_E_OFFSET;
				newValue = gcode.valueE;
		}
	
		// Check if G-code has parameter
		if(gcode.commandParameters & parameterOffset) {
	
			// Set new value
			if(mode == RELATIVE)
				newValue += currentValues[i];
			
			// Check if motor moves
			float distanceTraveled = fabs(newValue - currentValues[i]);
			if(distanceTraveled) {
			
				// Set lower new value
				bool lowerNewValue = newValue < currentValues[i];
				
				// Set current value
				currentValues[i] = newValue;
		
				// Set steps per mm, motor direction, speed limit, min/max feed rates, and saved value offset
				float stepsPerMm;
				float speedLimit;
				float maxFeedRate;
				float minFeedRate;
				eeprom_addr_t savedValueOffset = EEPROM_SIZE;
				switch(i) {
				
					case X:
						
						// Check if direction changed
						if(ioport_get_pin_level(MOTOR_X_DIRECTION_PIN) != (lowerNewValue ? DIRECTION_LEFT : DIRECTION_RIGHT))
						
							// Set backlash direction X
							backlashDirectionX = lowerNewValue ? NEGATIVE : POSITIVE;
						
						stepsPerMm = MOTOR_X_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, lowerNewValue ? DIRECTION_LEFT : DIRECTION_RIGHT);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_X_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_X_LENGTH);
						maxFeedRate = MOTOR_X_MAX_FEEDRATE;
						minFeedRate = MOTOR_X_MIN_FEEDRATE;
						
						savedValueOffset = EEPROM_SAVED_X_STATE_OFFSET;
					break;
					
					case Y:
					
						// Check if direction changed
						if(ioport_get_pin_level(MOTOR_Y_DIRECTION_PIN) != (lowerNewValue ? DIRECTION_FORWARD : DIRECTION_BACKWARD))
						
							// Set backlash direction Y
							backlashDirectionY = lowerNewValue ? NEGATIVE : POSITIVE;
					
						stepsPerMm = MOTOR_Y_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, lowerNewValue ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Y_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_Y_LENGTH);
						maxFeedRate = MOTOR_Y_MAX_FEEDRATE;
						minFeedRate = MOTOR_Y_MIN_FEEDRATE;
						
						savedValueOffset = EEPROM_SAVED_Y_STATE_OFFSET;
					break;
					
					case Z:
					
						stepsPerMm = MOTOR_Z_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, lowerNewValue ? DIRECTION_DOWN : DIRECTION_UP);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Z_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_Z_LENGTH);
						maxFeedRate = MOTOR_Z_MAX_FEEDRATE;
						minFeedRate = MOTOR_Z_MIN_FEEDRATE;
						
						savedValueOffset = EEPROM_SAVED_Z_STATE_OFFSET;
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
				
				// Check if motor moves
				if((motorMoves[i] = round(distanceTraveled * stepsPerMm * MICROSTEPS_PER_STEP))) {
				
					// Check if saving changes and motor has a saved state
					if(tasks & SAVE_CHANGES_TASK && savedValueOffset < EEPROM_SIZE) {
				
						// Set if value was valid
						validValues[i] = nvm_eeprom_read_byte(savedValueOffset);

						// Save that value is invalid
						nvm_eeprom_write_byte(savedValueOffset, INVALID);
					}
				
					// Set motor feedrate
					float motorFeedRate = min(currentValues[F], speedLimit);
				
					// Enforce min/max feed rates
					motorFeedRate = min(motorFeedRate, maxFeedRate);
					motorFeedRate = max(motorFeedRate, minFeedRate);
		
					// Set motor total time
					uint32_t motorTotalTime = round(distanceTraveled / motorFeedRate * 60 * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD);
		
					// Set slowest time
					slowestTime = max(motorTotalTime, slowestTime);
				}
			}
		}
	}
	
	// Check if set to compensate for backlash and it's applicable
	if(tasks & BACKLASH_TASK && (backlashDirectionX != NONE || backlashDirectionY != NONE))
	
		// Compensate for backlash
		compensateForBacklash(backlashDirectionX, backlashDirectionY);
	
	// Check if set to compensate for bed leveling
	if(tasks & BED_LEVELING_TASK)
		
		// Compensate for bed leveling
		compensateForBedLeveling();
	
	// Otherwise check if an emergency stop didn't happen
	else if(!emergencyStopOccured) {
	
		// Initialize variables
		uint32_t motorsTotalRoundedTime[NUMBER_OF_MOTORS] = {};
		uint32_t slowestRoundedTime = 0;
	
		// Go through all motors
		for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++)
	
			// Check if motor moves
			if(motorMoves[i]) {
			
				// Set motor number of steps
				motorsNumberOfSteps[i] = motorMoves[i];
		
				// Set motor step delay
				motorsStepDelayCounter[i] = 0;
				motorsStepDelay[i] = round(static_cast<float>(slowestTime) / motorsNumberOfSteps[i]);
		
				// Set motor total rounded time
				motorsTotalRoundedTime[i] = motorsNumberOfSteps[i] * motorsStepDelay[i];
		
				// Set slowest rounded time
				slowestRoundedTime = max(slowestRoundedTime, motorsTotalRoundedTime[i]);
		
				// Enable motor step interrupt and set motor Vref to active
				void (*setMotorStepInterruptLevel)(volatile void *tc, TC_INT_LEVEL_t level);
				switch(i) {
		
					case X:
						setMotorStepInterruptLevel = tc_set_cca_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
					break;
			
					case Y:
						setMotorStepInterruptLevel = tc_set_ccb_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
					break;
			
					case Z:
						setMotorStepInterruptLevel = tc_set_ccc_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
					break;
			
					default:
						setMotorStepInterruptLevel = tc_set_ccd_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_VREF_VOLTAGE_INITIAL / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
				}
				(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
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
		tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
		
		// Get next segment values
		getNextSegmentValues();
	
		// Wait until all motors step interrupts have stopped or an emergency stop occurs
		while(MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm) && !emergencyStopOccured) {
	
			// Check if E motor is moving
			if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm) {
		
				// Prevent updating temperature
				tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);
		
				// Read actual motor E voltages
				adc_write_configuration(&MOTOR_E_CURRENT_SENSE_ADC, &currentSenseAdcController);
				adcch_write_configuration(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL, &currentSenseAdcChannel);
				
				uint32_t value = 0;
				for(uint8_t i = 0; MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm && i < 50; i++) {
					adc_start_conversion(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
					adc_wait_for_interrupt_flag(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
					value += adc_get_unsigned_result(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
				}
				
				// Allow updating temperature
				tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
				
				// Check if motor E is still moving
				if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm) {
			
					// Set average actual motor E voltage
					value /= 50;
					float actualVoltage = ADC_VREF / (pow(2, 12) - 1) * value;
			
					// Get ideal motor E voltage
					float idealVoltage = static_cast<float>(tc_read_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL)) / MOTORS_VREF_TIMER_PERIOD * MICROCONTROLLER_VOLTAGE;
					
					// Adjust motor E Vref to maintain a constant motor current
					tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round((MOTOR_E_VREF_VOLTAGE_ACTIVE + idealVoltage - actualVoltage) / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
				}
			}
		}
	
		// Stop motors step timer
		tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
		
		// Set motors Vref to idle
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	}
	
	// Check if saving changes
	if(tasks & SAVE_CHANGES_TASK) {
	
		// Check if motor X direction changed
		if(backlashDirectionX != NONE)
		
			// Save motor X direction
			nvm_eeprom_write_byte(EEPROM_LAST_RECORDED_X_DIRECTION_OFFSET, backlashDirectionX == NEGATIVE ? DIRECTION_LEFT : DIRECTION_RIGHT);
		
		// Check if motor Y direction changed
		if(backlashDirectionY != NONE)
		
			// Save motor Y direction
			nvm_eeprom_write_byte(EEPROM_LAST_RECORDED_Y_DIRECTION_OFFSET, backlashDirectionY == NEGATIVE ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
		
		// Go through X, Y, and Z motors
		for(uint8_t i = 0; i < 3; i++)
		
			// Check if motor moved
			if(motorMoves[i]) {
			
				// Save current value and get saved value offset
				eeprom_addr_t savedValueOffset;
				switch(i) {
				
					case X:
						nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_X_VALUE_OFFSET, &currentValues[X], EEPROM_LAST_RECORDED_X_VALUE_LENGTH);
						savedValueOffset = EEPROM_SAVED_X_STATE_OFFSET;
					break;
					
					case Y:
						nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Y_VALUE_OFFSET, &currentValues[Y], EEPROM_LAST_RECORDED_Y_VALUE_LENGTH);
						savedValueOffset = EEPROM_SAVED_Y_STATE_OFFSET;
					break;
					
					default:
						nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
						savedValueOffset = EEPROM_SAVED_Z_STATE_OFFSET;
				}

				// Check if value was previously valid and an emergency stop didn't happen
				if(validValues[i] && !emergencyStopOccured)
	
					// Save that Z is valid
					nvm_eeprom_write_byte(savedValueOffset, VALID);
			}
	}
}

void Motors::moveToHeight(float height) {

	// Save mode
	MODES savedMode = mode;
	
	// Set mode to absolute
	mode = ABSOLUTE;
	
	// Move to Z value
	Gcode gcode;
	gcode.valueG = 0;
	gcode.valueZ = height;
	gcode.valueF = 90;
	gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_Z_OFFSET | PARAMETER_F_OFFSET;
	move(gcode, SAVE_CHANGES_TASK);
	
	// Restore mode
	mode = savedMode;
}

void Motors::compensateForBacklash(BACKLASH_DIRECTION backlashDirectionX, BACKLASH_DIRECTION backlashDirectionY) {

	// Save mode
	MODES savedMode = mode;
	
	// Set mode to relative
	mode = RELATIVE;
	
	// Save X, Y, and F values
	float savedX = currentValues[X];
	float savedY = currentValues[Y];
	float savedF = currentValues[F];
	
	// Initialize G-code
	Gcode gcode;
	gcode.valueG = gcode.valueX = gcode.valueY = 0;
	gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_F_OFFSET;
	
	// Set backlash X
	if(backlashDirectionX != NONE) {
		nvm_eeprom_read_buffer(EEPROM_BACKLASH_X_OFFSET, &gcode.valueX, EEPROM_BACKLASH_X_LENGTH);
		if(backlashDirectionX == NEGATIVE)
			gcode.valueX *= -1;
	}
	
	// Set backlash Y
	if(backlashDirectionY != NONE) {
		nvm_eeprom_read_buffer(EEPROM_BACKLASH_Y_OFFSET, &gcode.valueY, EEPROM_BACKLASH_Y_LENGTH);
		if(backlashDirectionY == NEGATIVE)
			gcode.valueY *= -1;
	}
	
	// Set backlash speed
	nvm_eeprom_read_buffer(EEPROM_BACKLASH_SPEED_OFFSET, &gcode.valueF, EEPROM_BACKLASH_SPEED_LENGTH);
	
	// Move by backlash amount
	move(gcode, NO_TASK);
	
	// Restore X, Y, and F values
	currentValues[X] = savedX;
	currentValues[Y] = savedY;
	currentValues[F] = savedF;
	
	// Restore mode
	mode = savedMode;
}

void Motors::compensateForBedLeveling() {

	// Save mode
	MODES savedMode = mode;
	
	// Set mode to absolute
	mode = ABSOLUTE;
	
	// Go through all motors
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
	
		// Set end values
		endValues[i] = currentValues[i];
		
		// Set current values to the start of the segment
		currentValues[i] = startValues[i];
		
		// Set value changes
		valueChanges[i] = endValues[i] - startValues[i];
	}
	
	// Get horizontal distance
	float horizontalDistance = sqrt(pow(valueChanges[X], 2) + pow(valueChanges[Y], 2));
	
	// Set value changes to ratios of the horizontal distance
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++)
		valueChanges[i] = horizontalDistance ? valueChanges[i] / horizontalDistance : 0;
	
	// Update vectors
	float orientation, offset;
	nvm_eeprom_read_buffer(EEPROM_BED_ORIENTATION_BACK_RIGHT_OFFSET, &orientation, EEPROM_BED_ORIENTATION_BACK_RIGHT_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_BED_OFFSET_BACK_RIGHT_OFFSET, &offset, EEPROM_BED_OFFSET_BACK_RIGHT_LENGTH);
	backRightVector.z = orientation + offset;
	
	nvm_eeprom_read_buffer(EEPROM_BED_ORIENTATION_BACK_LEFT_OFFSET, &orientation, EEPROM_BED_ORIENTATION_BACK_LEFT_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_BED_OFFSET_BACK_LEFT_OFFSET, &offset, EEPROM_BED_OFFSET_BACK_LEFT_LENGTH);
	backLeftVector.z = orientation + offset;
	
	nvm_eeprom_read_buffer(EEPROM_BED_ORIENTATION_FRONT_LEFT_OFFSET, &orientation, EEPROM_BED_ORIENTATION_FRONT_LEFT_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_BED_OFFSET_FRONT_LEFT_OFFSET, &offset, EEPROM_BED_OFFSET_FRONT_LEFT_LENGTH);
	frontLeftVector.z = orientation + offset;
	
	nvm_eeprom_read_buffer(EEPROM_BED_ORIENTATION_FRONT_RIGHT_OFFSET, &orientation, EEPROM_BED_ORIENTATION_FRONT_RIGHT_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_BED_OFFSET_FRONT_RIGHT_OFFSET, &offset, EEPROM_BED_OFFSET_FRONT_RIGHT_LENGTH);
	frontRightVector.z = orientation + offset;
	
	// Update planes
	backPlane = generatePlaneEquation(backLeftVector, backRightVector, centerVector);
	leftPlane = generatePlaneEquation(backLeftVector, frontLeftVector, centerVector);
	rightPlane = generatePlaneEquation(backRightVector, frontRightVector, centerVector);
	frontPlane = generatePlaneEquation(frontLeftVector, frontRightVector, centerVector);
	
	// Adjust current Z value for current real height
	currentValues[Z] += bedHeightOffset + getHeightAdjustmentRequired(currentValues[X], currentValues[Y]);
	
	// Update bed height offset
	nvm_eeprom_read_buffer(EEPROM_BED_HEIGHT_OFFSET_OFFSET, &bedHeightOffset, EEPROM_BED_HEIGHT_OFFSET_LENGTH);
	
	// Go through all segments
	for(numberOfSegments = max(1, ceil(horizontalDistance / SEGMENT_LENGTH)), segmentCounter = 0; segmentCounter <= numberOfSegments;) {
	
		// Set initial segment values
		if(!segmentCounter)
			getNextSegmentValues();
		
		// Move to end of current segment
		move(bedLevelingGcode, NO_TASK);
	}
	
	// Restore X, Y, Z, and E values
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++)
		currentValues[i] = endValues[i];
	
	// Restore mode
	mode = savedMode;
}

void Motors::getNextSegmentValues() {

	// Increment segment counter
	segmentCounter++;
	
	// Go through all motors
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
	
		// Set segment value
		float segmentValue = segmentCounter != numberOfSegments ? startValues[i] + segmentCounter * SEGMENT_LENGTH * valueChanges[i] : endValues[i];
	
		// Set G-code parameter
		switch(i) {
		
			case X:
				bedLevelingGcode.valueX = segmentValue;
			break;
			
			case Y:
				bedLevelingGcode.valueY = segmentValue;
			break;
			
			case Z:
				bedLevelingGcode.valueZ = segmentValue + bedHeightOffset + getHeightAdjustmentRequired(bedLevelingGcode.valueX, bedLevelingGcode.valueY);
			break;
			
			default:
				bedLevelingGcode.valueE = segmentValue;
		}
	}
}

void Motors::homeXY() {

	// Check if using legacy homing
	#ifdef LEGACY_HOMING
	
		// Save mode
		MODES savedMode = mode;
	
		// Set mode to relative
		mode = RELATIVE;
		
		// Move to corner
		Gcode gcode;
		gcode.valueG = 0;
		gcode.valueX = 112;
		gcode.valueY = 111;
		gcode.valueF = 3000;
		gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_F_OFFSET;
		move(gcode, SAVE_CHANGES_TASK);
	
		// Check if emergency stop hasn't occured
		if(!emergencyStopOccured) {
		
			// Move to center
			gcode.valueX = -54;
			gcode.valueY = -50;
			move(gcode, SAVE_CHANGES_TASK);
		
			// Set current X and Y
			currentValues[X] = 54;
			currentValues[Y] = 50;
			
			// Save current X and Y
			nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_X_VALUE_OFFSET, &currentValues[X], EEPROM_LAST_RECORDED_X_VALUE_LENGTH);
			nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Y_VALUE_OFFSET, &currentValues[Y], EEPROM_LAST_RECORDED_Y_VALUE_LENGTH);
			
			// Check if an emergency stop didn't happen
			if(!emergencyStopOccured) {

				// Save that X and Y are valid
				nvm_eeprom_write_byte(EEPROM_SAVED_X_STATE_OFFSET, VALID);
				nvm_eeprom_write_byte(EEPROM_SAVED_Y_STATE_OFFSET, VALID);
			}
		}
	
		// Restore mode
		mode = savedMode;
	
	// Otherwise
	#else
	
		// Save that X and Y are invalid
		nvm_eeprom_write_byte(EEPROM_SAVED_X_STATE_OFFSET, INVALID);
		nvm_eeprom_write_byte(EEPROM_SAVED_Y_STATE_OFFSET, INVALID);
		
		// Go through X and Y motors
		for(int8_t i = 1; i >= 0 && !emergencyStopOccured; i--) {
		
			// Delay
			delay_ms(100);
		
			// Set up motors to move all the way to the back as a fallback
			motorsDelaySkips[i] = 0;
			motorsStepDelay[i]  = 1;
			int16_t *accelerometerValue;
			int8_t jerkValue;
			void (*setMotorStepInterruptLevel)(volatile void *tc, TC_INT_LEVEL_t level);
			if(i) {
				motorsNumberOfSteps[i] = round(111 * MOTOR_Y_STEPS_PER_MM * MICROSTEPS_PER_STEP);
				ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, DIRECTION_BACKWARD);
				setMotorStepInterruptLevel = tc_set_ccb_interrupt_level;
				
				// Set accelerometer and jerk values
				accelerometerValue = &accelerometer.yAcceleration;
				jerkValue = 80;
				
				// Set motor Y Vref to active
				tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
			}
			else {
				motorsNumberOfSteps[i] = round(112 * MOTOR_X_STEPS_PER_MM * MICROSTEPS_PER_STEP);
				ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, DIRECTION_RIGHT);
				setMotorStepInterruptLevel = tc_set_cca_interrupt_level;
				
				// Set accelerometer and jerk values
				accelerometerValue = &accelerometer.xAcceleration;
				jerkValue = 65;
				
				// Set motor X Vref to active
				tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
			}
			
			// Enable motor step interrupt 
			(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
			
			// Turn on motors
			turnOn();

			// Start motors step timer
			tc_write_count(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD - 1);
			tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);

			// Wait until all motors step interrupts have stopped or an emergency stop occurs
			int16_t lastValue;
			uint8_t counter = 0;
			for(bool firstRun = true; MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm) && !emergencyStopOccured; firstRun = false) {
	
				// Get accelerometer values
				accelerometer.readAccelerationValues();
				if(!firstRun) {
	
					// Check if at the edge
					if(abs(lastValue - *accelerometerValue) >= jerkValue) {
						if(++counter >= 3)
			
							// Stop motor interrupt
							(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
					}
					else
						counter = 0;
				}
	
				// Save accelerometer values
				lastValue = *accelerometerValue;
			}

			// Stop motors step timer
			tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
			
			// Set motors Vref to idle
			tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
			tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		}
		
		// Check if emergency stop hasn't occured
		if(!emergencyStopOccured) {

			// Save mode
			MODES savedMode = mode;
			
			// Set mode to relative
			mode = RELATIVE;

			// Move to center
			Gcode gcode;
			gcode.valueG = 0;
			gcode.valueX = -54;
			gcode.valueY = -50;
			gcode.valueF = 3000;
			gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_F_OFFSET;
			move(gcode, SAVE_CHANGES_TASK);
	
			// Restore mode
			mode = savedMode;
	
			// Set current X and Y
			currentValues[X] = 54;
			currentValues[Y] = 50;
			
			// Save current X and Y
			nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_X_VALUE_OFFSET, &currentValues[X], EEPROM_LAST_RECORDED_X_VALUE_LENGTH);
			nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Y_VALUE_OFFSET, &currentValues[Y], EEPROM_LAST_RECORDED_Y_VALUE_LENGTH);
			
			// Check if an emergency stop didn't happen
			if(!emergencyStopOccured) {

				// Save that X and Y are valid
				nvm_eeprom_write_byte(EEPROM_SAVED_X_STATE_OFFSET, VALID);
				nvm_eeprom_write_byte(EEPROM_SAVED_Y_STATE_OFFSET, VALID);
			}
		}
	#endif
}

void Motors::saveZAsBedCenterZ0() {

	// Set current Z
	currentValues[Z] = 0.0999;

	// Save current Z
	nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);
	
	// Save that Z is valid
	nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, VALID);
}

void Motors::moveToZ0() {
	
	// Check if Z is valid
	bool validZ = nvm_eeprom_read_byte(EEPROM_SAVED_Z_STATE_OFFSET);
	
	// Save that Z is invalid
	nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, INVALID);
	
	// Find Z0
	float lastZ0 = currentValues[Z];
	float heighest = currentValues[Z] + 2;
	uint8_t matchCounter = 0;
	while(!emergencyStopOccured) {
	
		// Set up motors to move down
		motorsDelaySkips[Z] = 0;
		motorsStepDelay[Z] = 4;
		motorsNumberOfSteps[Z] = UINT32_MAX;
		ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, DIRECTION_DOWN);
		tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		
		// Set motor Z Vref to active
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_ACTIVE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		
		// Turn on motors
		turnOn();
		
		// Start motors step timer
		tc_write_count(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD - 1);
		tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
	
		// TODO Wait until all motors step interrupts have stopped or an emergency stop occurs
		int16_t lastZ;
		uint8_t counterZ = 0;
		for(bool firstRun = true; MOTORS_STEP_TIMER.INTCTRLB & TC0_CCCINTLVL_gm && !emergencyStopOccured; firstRun = false) {
		
			// Get accelerometer values
			accelerometer.readAccelerationValues();
			if(!firstRun) {
			
				/*char responseBuffer[255];
				char numberBuffer[sizeof("18446744073709551615")];
				strcpy(responseBuffer, "ok\t");
				lltoa(accelerometer.xAcceleration, numberBuffer);
				strcat(responseBuffer, numberBuffer);
				strcat(responseBuffer, "\t");
				lltoa(accelerometer.yAcceleration, numberBuffer);
				strcat(responseBuffer, numberBuffer);
				strcat(responseBuffer, "\t");
				lltoa(accelerometer.zAcceleration, numberBuffer);
				strcat(responseBuffer, numberBuffer);
				strcat(responseBuffer, "\r\n");
				sendDataToUsb(responseBuffer);*/
		
				// Check if motor Z has hit the bed
				if(abs(lastZ - accelerometer.zAcceleration) >= 3) {
					if(++counterZ >= 1)
				
						// Stop motor Z interrupt
						tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
				}
				else
					counterZ = 0;
			}
		
			// Save accelerometer values
			lastZ = accelerometer.zAcceleration;
		}
	
		// Stop motors step timer
		tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
		
		// Set current Z
		currentValues[Z] -= (static_cast<float>(UINT32_MAX) - motorsNumberOfSteps[Z]) / (MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP);
		
		// Check if emergency stop has occured
		if(emergencyStopOccured)
		
			// Break
			break;
		
		// Check if at the real Z0
		if(fabs(lastZ0 - currentValues[Z]) <= 1) {
		
			if(++matchCounter >= 2)
			
				// Break
				break;
		}
		else
			matchCounter = 0;
		
		// Save current Z as last Z0
		lastZ0 = currentValues[Z];
		
		// Move slightly up
		heighest = min(heighest, currentValues[Z] + 2);
		moveToHeight(heighest);
	}
	
	// Set motor Z Vref to idle
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_VREF_VOLTAGE_IDLE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	
	// Save current Z
	nvm_eeprom_erase_and_write_buffer(EEPROM_LAST_RECORDED_Z_VALUE_OFFSET, &currentValues[Z], EEPROM_LAST_RECORDED_Z_VALUE_LENGTH);

	// Check if Z was previously valid and an emergency stop didn't happen
	if(validZ && !emergencyStopOccured)
	
		// Save that Z is valid
		nvm_eeprom_write_byte(EEPROM_SAVED_Z_STATE_OFFSET, VALID);
}

void Motors::calibrateBedCenterZ0() {

	// Move to height 3mm
	moveToHeight(3);
	
	// Check if emergency stop hasn't occured
	if(!emergencyStopOccured) {

		// Home XY
		homeXY();
	
		// Check if emergency stop hasn't occured
		if(!emergencyStopOccured) {

			// Move to Z0
			moveToZ0();
		
			// Check if emergency stop hasn't occured
			if(!emergencyStopOccured) {

				// Save Z as bed center Z0
				saveZAsBedCenterZ0();
			
				// Move to height 3mm
				moveToHeight(3);
			}
		}
	}
}

void Motors::calibrateBedOrientation() {
	
	// Calibrate bed center Z0
	calibrateBedCenterZ0();
	
	// Initialize X and Y positions
	uint8_t positionsX[] = {9, 99, 99, 9};
	uint8_t positionsY[] = {5, 5, 95, 95};

	// Save mode
	MODES savedMode = mode;
	mode = ABSOLUTE;
	
	// Go through all corners
	for(uint8_t i = 0; i < 4 && !emergencyStopOccured; i++) {

		// Move to  corner
		Gcode gcode;
		gcode.valueG = 0;
		gcode.valueX = positionsX[i];
		gcode.valueY = positionsY[i];
		gcode.valueF = 3000;
		gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_F_OFFSET;
		move(gcode, BACKLASH_TASK | SAVE_CHANGES_TASK);
		
		// Check if emergency stop has occured
		if(emergencyStopOccured)
		
			// Break
			break;

		// Move to Z0
		moveToZ0();
		
		// Check if emergency stop has occured
		if(emergencyStopOccured)
		
			// Break
			break;
		
		// Get corner orientation offset and length
		eeprom_addr_t eepromOffset = EEPROM_BED_ORIENTATION_BACK_LEFT_OFFSET;
		uint8_t eepromLength = EEPROM_BED_ORIENTATION_BACK_LEFT_LENGTH;
		switch(i) {
		
			case 0:
				eepromOffset = EEPROM_BED_ORIENTATION_FRONT_LEFT_OFFSET;
				eepromLength = EEPROM_BED_ORIENTATION_FRONT_LEFT_LENGTH;
			break;
			
			case 1:
				eepromOffset = EEPROM_BED_ORIENTATION_FRONT_RIGHT_OFFSET;
				eepromLength = EEPROM_BED_ORIENTATION_FRONT_RIGHT_LENGTH;
			break;
			
			case 2:
				eepromOffset = EEPROM_BED_ORIENTATION_BACK_RIGHT_OFFSET;
				eepromLength = EEPROM_BED_ORIENTATION_BACK_RIGHT_LENGTH;
		}
		
		// Save corner orientation
		nvm_eeprom_erase_and_write_buffer(eepromOffset, &currentValues[Z], eepromLength);
	
		// Move to height 3mm
		moveToHeight(3);
	}
	
	// Restore mode
	mode = savedMode;
}

void Motors::reset() {

	// Stop motors step timer
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);

	// Turn off motors
	turnOff();

	// Disable all motor step interrupts
	MOTORS_STEP_TIMER.INTCTRLB &= ~(TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm);
	
	// Clear emergency stop occured
	emergencyStopOccured = false;
}
