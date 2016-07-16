// DRV8834 motor driver http://www.ti.com/lit/ds/slvsb19d/slvsb19d.pdf
// 24BYJ-48 5V 1/64 ratio stepper motor used in 4 wire bipolar mode, so just ignore the common wire (usually red) if it has one http://robocraft.ru/files/datasheet/28BYJ-48.pdf


// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include <math.h>
#include "motors.h"
#include "eeprom.h"
#include "heater.h"
#include "common.h"
#include "fan.h"


// Definitions
#define SEGMENT_LENGTH 2.0
#define HOMING_FEED_RATE 1500.0
#define CALIBRATING_Z_FEED_RATE 17.0
//#define REGULATE_EXTRUDER_CURRENT

// Bed dimensions
#define BED_CENTER_X 54.0
#define BED_CENTER_Y 50.0
#define BED_CENTER_X_DISTANCE_FROM_HOMING_CORNER 55.0
#define BED_CENTER_Y_DISTANCE_FROM_HOMING_CORNER 55.0
#define BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER 45.0
#define BED_LOW_MAX_X 106.0
#define BED_LOW_MIN_X -2.0
#define BED_LOW_MAX_Y 105.0
#define BED_LOW_MIN_Y -2.0
#define BED_LOW_MAX_Z 5.0
#define BED_LOW_MIN_Z 0.0
#define BED_MEDIUM_MAX_X 106.0
#define BED_MEDIUM_MIN_X -2.0
#define BED_MEDIUM_MAX_Y 105.0
#define BED_MEDIUM_MIN_Y -9.0
#define BED_MEDIUM_MAX_Z 73.5
#define BED_MEDIUM_MIN_Z BED_LOW_MAX_Z
#define BED_HIGH_MAX_X 97.0
#define BED_HIGH_MIN_X 7.0
#define BED_HIGH_MAX_Y 85.0
#define BED_HIGH_MIN_Y 9.0
#define BED_HIGH_MAX_Z 112.0
#define BED_HIGH_MIN_Z BED_MEDIUM_MAX_Z

// Motors settings
#define MICROSTEPS_PER_STEP 8
#define MOTORS_ENABLE_PIN IOPORT_CREATE_PIN(PORTB, 3)
#define MOTORS_STEP_CONTROL_PIN IOPORT_CREATE_PIN(PORTB, 2)
#define MOTORS_CURRENT_SENSE_RESISTANCE 0.1
#define MOTORS_CURRENT_TO_VOLTAGE_SCALAR (5.0 * MOTORS_CURRENT_SENSE_RESISTANCE)
#define MOTORS_SAVE_TIMER FAN_TIMER
#define MOTORS_SAVE_TIMER_PERIOD FAN_TIMER_PERIOD
#define MOTORS_STEP_TIMER TCC0
#define MOTORS_STEP_TIMER_PERIOD 0x400

// Motor X settings
#define MOTOR_X_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 2)
#define MOTOR_X_VREF_PIN IOPORT_CREATE_PIN(PORTD, 1)
#define MOTOR_X_STEP_PIN IOPORT_CREATE_PIN(PORTC, 5)
#define MOTOR_X_VREF_CHANNEL TC_CCB
#define MOTOR_X_VREF_CHANNEL_CAPTURE_COMPARE TC_CCBEN
#define MOTOR_X_CURRENT_IDLE 0.692018779
#define MOTOR_X_CURRENT_ACTIVE 0.723004695
#define MOTOR_X_STEPS_PER_MM 19.3067875

// Motor Y settings
#define MOTOR_Y_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 5)
#define MOTOR_Y_VREF_PIN IOPORT_CREATE_PIN(PORTD, 3)
#define MOTOR_Y_STEP_PIN IOPORT_CREATE_PIN(PORTC, 7)
#define MOTOR_Y_VREF_CHANNEL TC_CCD
#define MOTOR_Y_VREF_CHANNEL_CAPTURE_COMPARE TC_CCDEN
#define MOTOR_Y_CURRENT_IDLE 0.692018779
#define MOTOR_Y_CURRENT_ACTIVE 0.82629108
#define MOTOR_Y_STEPS_PER_MM 18.00885

// Motor Z settings
#define MOTOR_Z_DIRECTION_PIN IOPORT_CREATE_PIN(PORTD, 4)
#define MOTOR_Z_VREF_PIN IOPORT_CREATE_PIN(PORTD, 2)
#define MOTOR_Z_STEP_PIN IOPORT_CREATE_PIN(PORTC, 6)
#define MOTOR_Z_VREF_CHANNEL TC_CCC
#define MOTOR_Z_VREF_CHANNEL_CAPTURE_COMPARE TC_CCCEN
#define MOTOR_Z_CURRENT_IDLE 0.196244131
#define MOTOR_Z_CURRENT_ACTIVE 0.650704225
#define MOTOR_Z_STEPS_PER_MM 646.3295

// Motor E settings
#define MOTOR_E_DIRECTION_PIN IOPORT_CREATE_PIN(PORTC, 3)
#define MOTOR_E_VREF_PIN IOPORT_CREATE_PIN(PORTD, 0)
#define MOTOR_E_STEP_PIN IOPORT_CREATE_PIN(PORTC, 4)
#define MOTOR_E_CURRENT_SENSE_ADC ADC_MODULE
#define MOTOR_E_CURRENT_SENSE_PIN IOPORT_CREATE_PIN(PORTA, 7)
#define MOTOR_E_CURRENT_SENSE_ADC_FREQUENCY 200000
#define MOTOR_E_CURRENT_SENSE_ADC_SAMPLE_SIZE 50
#define MOTOR_E_CURRENT_SENSE_ADC_CHANNEL ADC_CH0
#define MOTOR_E_CURRENT_SENSE_ADC_PIN ADCCH_POS_PIN7
#define MOTOR_E_VREF_CHANNEL TC_CCA
#define MOTOR_E_VREF_CHANNEL_CAPTURE_COMPARE TC_CCAEN
#define MOTOR_E_CURRENT_IDLE 0.299530516
#define MOTOR_E_STEPS_PER_MM 128.451375

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
Motors *self;
uint32_t motorsDelaySkips[NUMBER_OF_MOTORS];
uint32_t motorsDelaySkipsCounter[NUMBER_OF_MOTORS];
uint32_t motorsStepDelay[NUMBER_OF_MOTORS];
uint32_t motorsStepDelayCounter[NUMBER_OF_MOTORS];
uint32_t motorsNumberOfSteps[NUMBER_OF_MOTORS];


// Supporting function implementation
void updateMotorsStepTimerInterrupts() {

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
}

void motorsStepTimerInterrupt(AXES motor) {

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
		if(motorsNumberOfSteps[motor]) {
		
			// Decrement step counter
			motorsNumberOfSteps[motor]--;
			
			// Set motor step pin
			ioport_set_pin_level(motorStepPin, IOPORT_PIN_LEVEL_HIGH);
		}

		// Check if done moving motor
		if(!motorsNumberOfSteps[motor])
		
			// Disable motor step interrupt
			(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
		
		// Clear motor step counter
		motorsStepDelayCounter[motor] = 0;
	}
}

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

void startMotorsStepTimer() {

	// Update motors step timer interrupts
	updateMotorsStepTimerInterrupts();
	
	// Restart motors step timer
	tc_restart(&MOTORS_STEP_TIMER);
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_DIV1_gc);
}

void stopMotorsStepTimer() {

	// Stop motors step timer
	tc_write_clock_source(&MOTORS_STEP_TIMER, TC_CLKSEL_OFF_gc);
	
	// Update motors step timer interrupts
	updateMotorsStepTimerInterrupts();
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

void Motors::initialize() {

	// Set self
	self = this;

	// Restore state
	restoreState();

	// Set mode
	mode = ABSOLUTE;
	
	// Set initial values
	currentValues[E] = 0;
	currentValues[F] = EEPROM_SPEED_LIMIT_X_MAX;
	
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
	ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, currentMotorDirections[X]);
	ioport_set_pin_dir(MOTOR_X_STEP_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure motor Y Vref, direction, and step
	ioport_set_pin_dir(MOTOR_Y_VREF_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(MOTOR_Y_DIRECTION_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, currentMotorDirections[Y]);
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
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	tc_enable_cc_channels(&MOTORS_VREF_TIMER, static_cast<tc_cc_channel_mask_enable_t>(MOTOR_X_VREF_CHANNEL_CAPTURE_COMPARE | MOTOR_Y_VREF_CHANNEL_CAPTURE_COMPARE | MOTOR_Z_VREF_CHANNEL_CAPTURE_COMPARE | MOTOR_E_VREF_CHANNEL_CAPTURE_COMPARE));
	tc_write_clock_source(&MOTORS_VREF_TIMER, TC_CLKSEL_DIV1_gc);
	
	// Configure motors step timer
	tc_enable(&MOTORS_STEP_TIMER);
	tc_set_wgm(&MOTORS_STEP_TIMER, TC_WG_SS);
	tc_write_period(&MOTORS_STEP_TIMER, MOTORS_STEP_TIMER_PERIOD);
	tc_set_overflow_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_MED);
	
	// Update motors step timer interrupts
	updateMotorsStepTimerInterrupts();
	
	// Motors step timer overflow callback
	tc_set_overflow_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
		
		// Update motors step timer interrupts
		updateMotorsStepTimerInterrupts();
	});
	
	// Motor X step timer callback
	tc_set_cca_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run motors step timer interrupt
		motorsStepTimerInterrupt(X);
	});
	
	// Motor Y step timer callback
	tc_set_ccb_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run motors step timer interrupt
		motorsStepTimerInterrupt(Y);
	});
	
	// Motor Z step timer callback
	tc_set_ccc_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run motors step timer interrupt
		motorsStepTimerInterrupt(Z);
	});
	
	// Motor E step timer callback
	tc_set_ccd_interrupt_callback(&MOTORS_STEP_TIMER, []() -> void {
	
		// Run motors step timer interrupt
		motorsStepTimerInterrupt(E);
	});
	
	// Set ADC controller to use unsigned, 12-bit, Vref refrence, and manual trigger
	adc_read_configuration(&MOTOR_E_CURRENT_SENSE_ADC, &currentSenseAdcController);
	adc_set_conversion_parameters(&currentSenseAdcController, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_AREFA);
	adc_set_conversion_trigger(&currentSenseAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&currentSenseAdcController, MOTOR_E_CURRENT_SENSE_ADC_FREQUENCY);
	
	// Set ADC channel to use motor E current sense pin as single ended input with no gain
	adcch_read_configuration(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL, &currentSenseAdcChannel);
	adcch_set_input(&currentSenseAdcChannel, MOTOR_E_CURRENT_SENSE_ADC_PIN, ADCCH_NEG_NONE, 1);
	
	// Initialize accelerometer
	accelerometer.initialize();
	
	// Set vectors
	backRightVector.initialize(BED_CENTER_X + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER);
	backLeftVector.initialize(BED_CENTER_X - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER);
	frontLeftVector.initialize(BED_CENTER_X - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER);
	frontRightVector.initialize(BED_CENTER_X + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER);
	centerVector.initialize(BED_CENTER_X, BED_CENTER_Y);
	
	// Update bed changes
	updateBedChanges(false);
	
	// Configure motors save interrupt
	tc_set_overflow_interrupt_callback(&MOTORS_SAVE_TIMER, []() -> void {
	
		// Initialize variables
		static uint8_t motorsSaveTimerCounter = 0;
		static AXES currentSaveMotor = Z;
		static AXES_PARAMETER currentSaveParameter = VALUE;
	
		// Check if one fifth of a second has passed
		if(++motorsSaveTimerCounter >= sysclk_get_cpu_hz() / MOTORS_SAVE_TIMER_PERIOD / 64 / 5) {
		
			// Reset motors save timer counter
			motorsSaveTimerCounter = 0;
			
			// Check if all parameters for the current motor have been saved
			if(currentSaveParameter == VALUE) {
			
				// Reset current save parameter
				currentSaveParameter = DIRECTION;

				// Set current save motor to next motor
				currentSaveMotor = currentSaveMotor == Z ? X : static_cast<AXES>(currentSaveMotor + 1);
			}
			
			// Otherwise
			else
			
				// Set current save parameter to next parameter
				currentSaveParameter = static_cast<AXES_PARAMETER>(currentSaveParameter + 1);
	
			// Save current motor's state
			self->saveState(currentSaveMotor, currentSaveParameter);
		}
	});
	tc_set_overflow_interrupt_level(&MOTORS_SAVE_TIMER, TC_INT_LVL_LO);
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
	float slowestTime = 0;
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
		
				// Set steps per mm, motor direction, speed limit, and min/max feed rates
				float stepsPerMm;
				float speedLimit;
				float maxFeedRate;
				float minFeedRate;
				bool savesValidValue = true;
				switch(i) {
				
					case X:
						
						// Check if direction changed
						if(currentMotorDirections[X] != (lowerNewValue ? DIRECTION_LEFT : DIRECTION_RIGHT))
						
							// Set backlash direction X
							backlashDirectionX = lowerNewValue ? NEGATIVE : POSITIVE;
						
						stepsPerMm = MOTOR_X_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, lowerNewValue ? DIRECTION_LEFT : DIRECTION_RIGHT);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_X_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_X_LENGTH);
						maxFeedRate = EEPROM_SPEED_LIMIT_X_MAX;
						minFeedRate = EEPROM_SPEED_LIMIT_X_MIN;
					break;
					
					case Y:
					
						// Check if direction changed
						if(currentMotorDirections[Y] != (lowerNewValue ? DIRECTION_FORWARD : DIRECTION_BACKWARD))
						
							// Set backlash direction Y
							backlashDirectionY = lowerNewValue ? NEGATIVE : POSITIVE;
					
						stepsPerMm = MOTOR_Y_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, lowerNewValue ? DIRECTION_FORWARD : DIRECTION_BACKWARD);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Y_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_Y_LENGTH);
						maxFeedRate = EEPROM_SPEED_LIMIT_Y_MAX;
						minFeedRate = EEPROM_SPEED_LIMIT_Y_MIN;
					break;
					
					case Z:
					
						stepsPerMm = MOTOR_Z_STEPS_PER_MM;
						ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, lowerNewValue ? DIRECTION_DOWN : DIRECTION_UP);
						nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_Z_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_Z_LENGTH);
						maxFeedRate = EEPROM_SPEED_LIMIT_Z_MAX;
						minFeedRate = EEPROM_SPEED_LIMIT_Z_MIN;
					break;
					
					default:
						stepsPerMm = MOTOR_E_STEPS_PER_MM;
						if(lowerNewValue) {
							ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, DIRECTION_RETRACT);
							nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_NEGATIVE_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_E_NEGATIVE_LENGTH);
							maxFeedRate = EEPROM_SPEED_LIMIT_E_NEGATIVE_MAX;
							minFeedRate = EEPROM_SPEED_LIMIT_E_NEGATIVE_MIN;
						}
						else {
							ioport_set_pin_level(MOTOR_E_DIRECTION_PIN, DIRECTION_EXTRUDE);
							nvm_eeprom_read_buffer(EEPROM_SPEED_LIMIT_E_POSITIVE_OFFSET, &speedLimit, EEPROM_SPEED_LIMIT_E_POSITIVE_LENGTH);
							maxFeedRate = EEPROM_SPEED_LIMIT_E_POSITIVE_MAX;
							minFeedRate = EEPROM_SPEED_LIMIT_E_POSITIVE_MIN;
						}
						
						// Clear saves valid value
						savesValidValue = false;
				}
				
				// Check if motor moves
				if((motorMoves[i] = round(distanceTraveled * stepsPerMm * MICROSTEPS_PER_STEP))) {
				
					// Check if saving changes and saves if motor's state is valid
					if(tasks & SAVE_CHANGES_TASK && savesValidValue) {
				
						// Save if value is valid
						validValues[i] = currentStateOfValues[i];

						// Set that value is invalid
						currentStateOfValues[i] = INVALID;
					}
				
					// Set motor feedrate
					float motorFeedRate = min(currentValues[F], speedLimit);
				
					// Enforce min/max feed rates
					motorFeedRate = getValueInRange(motorFeedRate, minFeedRate, maxFeedRate);
		
					// Set slowest time
					slowestTime = max(distanceTraveled / motorFeedRate * 60, slowestTime);
				}
			}
		}
	}
	
	// Check if set to compensate for backlash and it's applicable
	if(tasks & BACKLASH_TASK && (backlashDirectionX != NONE || backlashDirectionY != NONE))
	
		// Compensate for backlash
		compensateForBacklash(backlashDirectionX, backlashDirectionY);
	
	// Check if set to compensate for bed leveling
	if(tasks & BED_LEVELING_TASK) {
	
		// Limit X and Y from moving out of bounds
		float minValues[2], maxValues[2];
		if(currentValues[Z] < BED_LOW_MAX_Z) {
			minValues[X] = BED_LOW_MIN_X;
			minValues[Y] = BED_LOW_MIN_Y;
			maxValues[X] = BED_LOW_MAX_X;
			maxValues[Y] = BED_LOW_MAX_Y;
		}
		else if(currentValues[Z] < BED_MEDIUM_MAX_Z) {
			minValues[X] = BED_MEDIUM_MIN_X;
			minValues[Y] = BED_MEDIUM_MIN_Y;
			maxValues[X] = BED_MEDIUM_MAX_X;
			maxValues[Y] = BED_MEDIUM_MAX_Y;
		}
		else {
			minValues[X] = BED_HIGH_MIN_X;
			minValues[Y] = BED_HIGH_MIN_Y;
			maxValues[X] = BED_HIGH_MAX_X;
			maxValues[Y] = BED_HIGH_MAX_Y;
		}
		
		currentValues[X] = getValueInRange(currentValues[X], minValues[X], maxValues[X]);
		currentValues[Y] = getValueInRange(currentValues[Y], minValues[Y], maxValues[Y]);
		
		// Compensate for bed leveling
		compensateForBedLeveling();
	}
	
	// Otherwise check if an emergency stop didn't happen
	else if(!emergencyStopOccured) {
	
		// Initialize variables
		uint32_t motorsTotalRoundedTime[NUMBER_OF_MOTORS] = {};
		uint32_t slowestRoundedTime = 0;
		float motorVoltageE = 0;
	
		// Go through all motors
		for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++)
	
			// Check if motor moves
			if(motorMoves[i]) {
			
				// Set motor number of steps
				motorsNumberOfSteps[i] = motorMoves[i];
		
				// Set motor step delay
				motorsStepDelayCounter[i] = 0;
				motorsStepDelay[i] = minimumOneCeil(slowestTime * sysclk_get_cpu_hz() / MOTORS_STEP_TIMER_PERIOD / motorsNumberOfSteps[i]);
		
				// Set motor total rounded time
				motorsTotalRoundedTime[i] = motorsNumberOfSteps[i] * motorsStepDelay[i];
		
				// Set slowest rounded time
				slowestRoundedTime = max(motorsTotalRoundedTime[i], slowestRoundedTime);
		
				// Enable motor step interrupt and set motor Vref to active
				void (*setMotorStepInterruptLevel)(volatile void *tc, TC_INT_LEVEL_t level);
				switch(i) {
		
					case X:
						setMotorStepInterruptLevel = tc_set_cca_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_CURRENT_ACTIVE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
					break;
			
					case Y:
						setMotorStepInterruptLevel = tc_set_ccb_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_CURRENT_ACTIVE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
					break;
			
					case Z:
						setMotorStepInterruptLevel = tc_set_ccc_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_CURRENT_ACTIVE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
					break;
			
					default:
						
						// Set motor E voltage
						uint16_t motorCurrentE;
						nvm_eeprom_read_buffer(EEPROM_E_MOTOR_CURRENT_OFFSET, &motorCurrentE, EEPROM_E_MOTOR_CURRENT_LENGTH);
						motorVoltageE = MOTORS_CURRENT_TO_VOLTAGE_SCALAR / 1000 * motorCurrentE;
						
						setMotorStepInterruptLevel = tc_set_ccd_interrupt_level;
						tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(motorVoltageE / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
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
		
		// Check if regulating extruder current
		#ifdef REGULATE_EXTRUDER_CURRENT
		
			// Check if motor E moves
			if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm)
		
				// Wait enough time for motor E voltage to stabilize
				delay_us(500);
		#endif
	
		// Start motors step timer
		startMotorsStepTimer();
	
		// Wait until all motors step interrupts have stopped or an emergency stop occurs
		while(MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm) && !emergencyStopOccured) {
		
			// Check if regulating extruder current
			#ifdef REGULATE_EXTRUDER_CURRENT
	
				// Check if motor E is moving
				if(MOTORS_STEP_TIMER.INTCTRLB & TC0_CCDINTLVL_gm) {
		
					// Prevent updating temperature
					tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);
		
					// Read actual motor E voltages
					adc_write_configuration(&MOTOR_E_CURRENT_SENSE_ADC, &currentSenseAdcController);
					adcch_write_configuration(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL, &currentSenseAdcChannel);
				
					uint32_t value = 0;
					for(uint8_t i = 0; i < MOTOR_E_CURRENT_SENSE_ADC_SAMPLE_SIZE; i++) {
						adc_start_conversion(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
						adc_wait_for_interrupt_flag(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
						value += adc_get_unsigned_result(&MOTOR_E_CURRENT_SENSE_ADC, MOTOR_E_CURRENT_SENSE_ADC_CHANNEL);
					}
				
					// Allow updating temperature
					tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
				
					// Set average actual motor E voltage
					value /= MOTOR_E_CURRENT_SENSE_ADC_SAMPLE_SIZE;
					float actualVoltage = ADC_VREF_VOLTAGE / UINT12_MAX * value;
		
					// Get ideal motor E voltage
					float idealVoltage = static_cast<float>(tc_read_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL)) / MOTORS_VREF_TIMER_PERIOD * MICROCONTROLLER_VOLTAGE;
				
					// Adjust motor E voltage to maintain a constant motor current
					tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round((motorVoltageE + idealVoltage - actualVoltage) / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
				}
			#endif
		}
	
		// Stop motors step timer
		stopMotorsStepTimer();
		
		// Set motors Vref to idle
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_E_VREF_CHANNEL, round(MOTOR_E_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	}
	
	// Check if saving changes
	if(tasks & SAVE_CHANGES_TASK) {
	
		// Check if motor X direction changed
		if(backlashDirectionX != NONE)
		
			// Set motor X direction
			currentMotorDirections[X] = backlashDirectionX == NEGATIVE ? DIRECTION_LEFT : DIRECTION_RIGHT;
		
		// Check if motor Y direction changed
		if(backlashDirectionY != NONE)
		
			// Set motor Y direction
			currentMotorDirections[Y] = backlashDirectionY == NEGATIVE ? DIRECTION_FORWARD : DIRECTION_BACKWARD;
		
		// Go through X, Y, and Z motors
		for(uint8_t i = 0; i < 3; i++)
		
			// Check if motor moved, value was previously valid, and an emergency stop didn't happen
			if(motorMoves[i] && validValues[i] && !emergencyStopOccured)
				
				// Set that Z is valid
				currentStateOfValues[i] = VALID;
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
	gcode.valueF = EEPROM_SPEED_LIMIT_Z_MAX;
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
	
	// Adjust current Z value for current real height
	currentValues[Z] += getHeightAdjustmentRequired(startValues[X], startValues[Y]);
	
	// Get horizontal distance
	float horizontalDistance = sqrt(pow(valueChanges[X], 2) + pow(valueChanges[Y], 2));
	
	// Set value changes to ratios of the horizontal distance
	for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++)
		valueChanges[i] = horizontalDistance ? valueChanges[i] / horizontalDistance : 0;
	
	// Initialize G-code
	Gcode gcode;
	gcode.valueG = 0;
	gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_Z_OFFSET | PARAMETER_E_OFFSET;
	
	// Go through all segments
	for(uint32_t numberOfSegments = minimumOneCeil(horizontalDistance / SEGMENT_LENGTH), segmentCounter = 1; segmentCounter <= numberOfSegments; segmentCounter++) {
	
		// Go through all motors
		for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
	
			// Set segment value
			float segmentValue = segmentCounter != numberOfSegments ? startValues[i] + segmentCounter * SEGMENT_LENGTH * valueChanges[i] : endValues[i];
			
			// Set G-code parameter
			switch(i) {
		
				case X:
					gcode.valueX = segmentValue;
				break;
			
				case Y:
					gcode.valueY = segmentValue;
				break;
			
				case Z:
					gcode.valueZ = segmentValue + getHeightAdjustmentRequired(gcode.valueX, gcode.valueY);
				break;
			
				default:
					gcode.valueE = segmentValue;
			}
		}
		
		// Move to end of current segment
		move(gcode, NO_TASK);
	}
	
	// Restore Z value
	currentValues[Z] = endValues[Z];
	
	// Restore mode
	mode = savedMode;
}

void Motors::updateBedChanges(bool adjustHeight) {

	// Initialize bed height offset
	static float bedHeightOffset;

	// Set previous height adjustment
	float previousHeightAdjustment = getHeightAdjustmentRequired(currentValues[X], currentValues[Y]) + bedHeightOffset;
	
	// Go through all positions
	for(uint8_t i = 0; i < 4; i++) {
	
		// Set position's orientation, offset, and value
		eeprom_addr_t orientationOffset, offsetOffset;
		uint8_t orientationLength, offsetLength;
		float *value;
		switch(i) {
		
			case 0:
				orientationOffset = EEPROM_BED_ORIENTATION_BACK_RIGHT_OFFSET;
				orientationLength = EEPROM_BED_ORIENTATION_BACK_RIGHT_LENGTH;
				offsetOffset = EEPROM_BED_OFFSET_BACK_RIGHT_OFFSET;
				offsetLength = EEPROM_BED_OFFSET_BACK_RIGHT_LENGTH;
				value = &backRightVector.z;
			break;
			
			case 1:
				orientationOffset = EEPROM_BED_ORIENTATION_BACK_LEFT_OFFSET;
				orientationLength = EEPROM_BED_ORIENTATION_BACK_LEFT_LENGTH;
				offsetOffset = EEPROM_BED_OFFSET_BACK_LEFT_OFFSET;
				offsetLength = EEPROM_BED_OFFSET_BACK_LEFT_LENGTH;
				value = &backLeftVector.z;
			break;
			
			case 2:
				orientationOffset = EEPROM_BED_ORIENTATION_FRONT_LEFT_OFFSET;
				orientationLength = EEPROM_BED_ORIENTATION_FRONT_LEFT_LENGTH;
				offsetOffset = EEPROM_BED_OFFSET_FRONT_LEFT_OFFSET;
				offsetLength = EEPROM_BED_OFFSET_FRONT_LEFT_LENGTH;
				value = &frontLeftVector.z;
			break;
			
			default:
				orientationOffset = EEPROM_BED_ORIENTATION_FRONT_RIGHT_OFFSET;
				orientationLength = EEPROM_BED_ORIENTATION_FRONT_RIGHT_LENGTH;
				offsetOffset = EEPROM_BED_OFFSET_FRONT_RIGHT_OFFSET;
				offsetLength = EEPROM_BED_OFFSET_FRONT_RIGHT_LENGTH;
				value = &frontRightVector.z;
		}
		
		// Update position vector
		float orientation, offset;
		nvm_eeprom_read_buffer(orientationOffset, &orientation, orientationLength);
		nvm_eeprom_read_buffer(offsetOffset, &offset, offsetLength);
		*value = orientation + offset;
	}
	
	// Update planes
	backPlane = generatePlaneEquation(backLeftVector, backRightVector, centerVector);
	leftPlane = generatePlaneEquation(backLeftVector, frontLeftVector, centerVector);
	rightPlane = generatePlaneEquation(backRightVector, frontRightVector, centerVector);
	frontPlane = generatePlaneEquation(frontLeftVector, frontRightVector, centerVector);
	
	// Update bed height offset
	nvm_eeprom_read_buffer(EEPROM_BED_HEIGHT_OFFSET_OFFSET, &bedHeightOffset, EEPROM_BED_HEIGHT_OFFSET_LENGTH);
	
	// Check if adjusting height
	if(adjustHeight)
	
		// Set current Z
		currentValues[Z] += previousHeightAdjustment - getHeightAdjustmentRequired(currentValues[X], currentValues[Y]) - bedHeightOffset;
}

bool Motors::gantryClipsDetected() {

	// Return false
	return false;
}

void Motors::changeState(bool save, AXES motor, AXES_PARAMETER parameter) {

	// Go through X, Y, and Z motors
	for(uint8_t i = motor; i <= (save ? motor : 2); i++) {
	
		// Get value, state, and direction offsets
		eeprom_addr_t savedValueOffset, savedStateOffset, savedDirectionOffset = EEPROM_SIZE;
		uint8_t savedValueLength;
		switch(i) {
	
			case X:
				savedValueOffset = EEPROM_LAST_RECORDED_X_VALUE_OFFSET;
				savedValueLength = EEPROM_LAST_RECORDED_X_VALUE_LENGTH;
				savedStateOffset = EEPROM_SAVED_X_STATE_OFFSET;
				savedDirectionOffset = EEPROM_LAST_RECORDED_X_DIRECTION_OFFSET;
			break;
		
			case Y:
				savedValueOffset = EEPROM_LAST_RECORDED_Y_VALUE_OFFSET;
				savedValueLength = EEPROM_LAST_RECORDED_Y_VALUE_LENGTH;
				savedStateOffset = EEPROM_SAVED_Y_STATE_OFFSET;
				savedDirectionOffset = EEPROM_LAST_RECORDED_Y_DIRECTION_OFFSET;
			break;
		
			default:
				savedValueOffset = EEPROM_LAST_RECORDED_Z_VALUE_OFFSET;
				savedValueLength = EEPROM_LAST_RECORDED_Z_VALUE_LENGTH;
				savedStateOffset = EEPROM_SAVED_Z_STATE_OFFSET;
		}
		
		// Check if saving state
		if(save) {
		
			// Check if direction is being saved and it's not up to date
			if(parameter == DIRECTION && savedDirectionOffset < EEPROM_SIZE && nvm_eeprom_read_byte(savedDirectionOffset) != currentMotorDirections[i])
		
				// Save direction
				nvm_eeprom_write_byte(savedDirectionOffset, currentMotorDirections[i]);
			
			// Otherwise check if validity is being saved and it's not up to date
			else if(parameter == VALIDITY && nvm_eeprom_read_byte(savedStateOffset) != currentStateOfValues[i])
			
				// Save if value is valid
				nvm_eeprom_write_byte(savedStateOffset, currentStateOfValues[i]);
			
			// Otherwise check if value is being saved
			else if(parameter == VALUE) {
		
				// Check if current value is not up to date
				float value;
				nvm_eeprom_read_buffer(savedValueOffset, &value, savedValueLength);
				if(value != currentValues[i])
			
					// Save current value
					nvm_eeprom_erase_and_write_buffer(savedValueOffset, &currentValues[i], savedValueLength);
			}
		}
		
		// Otherwise assume restoring state
		else {
		
			// Check if direction is saved
			if(savedDirectionOffset < EEPROM_SIZE)
		
				// Restore current direction
				currentMotorDirections[i] = nvm_eeprom_read_byte(savedDirectionOffset);
			
			// Restore current state
			currentStateOfValues[i] = nvm_eeprom_read_byte(savedStateOffset);
			
			// Restore current value
			nvm_eeprom_read_buffer(savedValueOffset, &currentValues[i], savedValueLength);
		}
	}
}

void Motors::homeXY(bool adjustHeight) {

	// Set that X and Y are invalid
	currentStateOfValues[X] = currentStateOfValues[Y] = INVALID;
	
	// Go through X and Y motors
	for(int8_t i = 1; i >= 0 && !emergencyStopOccured; i--) {
	
		// Set up motors to move all the way to the back right corner as a fallback
		int16_t *accelerometerValue;
		int16_t jerkAcceleration;
		void (*setMotorStepInterruptLevel)(volatile void *tc, TC_INT_LEVEL_t level);
		float distance;
		if(i) {
			distance = max(max(BED_LOW_MAX_Y, BED_MEDIUM_MAX_Y), BED_HIGH_MAX_Y) - min(min(BED_LOW_MIN_Y, BED_MEDIUM_MIN_Y), BED_HIGH_MIN_Y) + 5;
			motorsNumberOfSteps[i] = round(distance * MOTOR_Y_STEPS_PER_MM * MICROSTEPS_PER_STEP);
			ioport_set_pin_level(MOTOR_Y_DIRECTION_PIN, DIRECTION_BACKWARD);
			setMotorStepInterruptLevel = tc_set_ccb_interrupt_level;
			
			// Set accelerometer value
			accelerometerValue = &accelerometer.yAcceleration;
			jerkAcceleration = Y_JERK_ACCELERATION;
			
			// Set motor Y Vref to active
			tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_CURRENT_ACTIVE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		}
		else {
			distance = max(max(BED_LOW_MAX_X, BED_MEDIUM_MAX_X), BED_HIGH_MAX_X) - min(min(BED_LOW_MIN_X, BED_MEDIUM_MIN_X), BED_HIGH_MIN_X) + 5;
			motorsNumberOfSteps[i] = round(distance * MOTOR_X_STEPS_PER_MM * MICROSTEPS_PER_STEP);
			ioport_set_pin_level(MOTOR_X_DIRECTION_PIN, DIRECTION_RIGHT);
			setMotorStepInterruptLevel = tc_set_cca_interrupt_level;
			
			// Set accelerometer value
			accelerometerValue = &accelerometer.xAcceleration;
			jerkAcceleration = X_JERK_ACCELERATION;
			
			// Set motor X Vref to active
			tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_CURRENT_ACTIVE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		}
		
		// Set motor delay to achieve desired feed rate
		motorsStepDelayCounter[i] = motorsDelaySkipsCounter[i] = 0;
		motorsStepDelay[i] = minimumOneCeil((distance * 60 * sysclk_get_cpu_hz()) / (HOMING_FEED_RATE * MOTORS_STEP_TIMER_PERIOD * motorsNumberOfSteps[i]));
		float denominator = (distance * 60 * sysclk_get_cpu_hz()) / (HOMING_FEED_RATE * MOTORS_STEP_TIMER_PERIOD * motorsNumberOfSteps[i] * motorsStepDelay[i]) - 1;
		motorsDelaySkips[i] = denominator ? round(1 / denominator) : 0;
		
		// Enable motor step interrupt 
		(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		
		// Turn on motors
		turnOn();

		// Start motors step timer
		startMotorsStepTimer();

		// Wait until all motors step interrupts have stopped or an emergency stop occurs
		int16_t lastValue;
		uint8_t counter = 0;
		for(bool firstRun = true; MOTORS_STEP_TIMER.INTCTRLB & (TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm) && !emergencyStopOccured; firstRun = false) {

			// Get accelerometer values
			accelerometer.readAccelerationValues();
			
			// Check if not first run
			if(!firstRun) {
			
				// Check if extruder hit the edge
				if(abs(lastValue - *accelerometerValue) >= jerkAcceleration) {
					if(++counter >= 2)
	
						// Stop motor interrupt
						(*setMotorStepInterruptLevel)(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
				}
				else
					counter = 0;
			}
			
			// Save value
			lastValue = *accelerometerValue;
		}

		// Stop motors step timer
		stopMotorsStepTimer();
		
		// Set motors Vref to idle
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_X_VREF_CHANNEL, round(MOTOR_X_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Y_VREF_CHANNEL, round(MOTOR_Y_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
	}
	
	// Check if emergency stop hasn't occured
	if(!emergencyStopOccured) {
	
		// Save current Z
		float savedZ = currentValues[Z];

		// Save mode
		MODES savedMode = mode;
		
		// Set mode to relative
		mode = RELATIVE;
		
		// Move to center
		Gcode gcode;
		gcode.valueG = 0;
		gcode.valueX = -BED_CENTER_X_DISTANCE_FROM_HOMING_CORNER;
		gcode.valueY = -BED_CENTER_Y_DISTANCE_FROM_HOMING_CORNER;
		gcode.valueZ = adjustHeight ? getHeightAdjustmentRequired(BED_CENTER_X, BED_CENTER_Y) - getHeightAdjustmentRequired(currentValues[X], currentValues[Y]) : 0;
		gcode.valueF = EEPROM_SPEED_LIMIT_X_MAX;
		gcode.commandParameters = PARAMETER_G_OFFSET | PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_Z_OFFSET | PARAMETER_F_OFFSET;
		move(gcode, SAVE_CHANGES_TASK);

		// Restore mode
		mode = savedMode;

		// Set current X, Y, and Z
		currentValues[X] = BED_CENTER_X;
		currentValues[Y] = BED_CENTER_Y;
		currentValues[Z] = savedZ;
		
		// Check if an emergency stop didn't happen
		if(!emergencyStopOccured)

			// Set that X and Y are valid
			currentStateOfValues[X] = currentStateOfValues[Y] = VALID;
	}
}

void Motors::saveZAsBedCenterZ0() {

	// Set current Z
	currentValues[Z] = 0;
	
	// Set that Z is valid
	currentStateOfValues[Z] = VALID;
}

void Motors::moveToZ0() {
	
	// Save if Z is valid
	bool validZ = currentStateOfValues[Z];
	
	// Set that Z is invalid
	currentStateOfValues[Z] = INVALID;
	
	// Find Z0
	float lastZ0 = currentValues[Z];
	for(uint8_t matchCounter = 0; !emergencyStopOccured;) {
	
		// Set up motors to move down
		motorsNumberOfSteps[Z] = UINT32_MAX;
		ioport_set_pin_level(MOTOR_Z_DIRECTION_PIN, DIRECTION_DOWN);
		tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_LO);
		
		// Set motor delay to achieve desired feed rate
		motorsStepDelayCounter[Z] = motorsDelaySkipsCounter[Z] = 0;
		motorsStepDelay[Z] = minimumOneCeil(((motorsNumberOfSteps[Z] / (MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP)) * 60 * sysclk_get_cpu_hz()) / (CALIBRATING_Z_FEED_RATE * MOTORS_STEP_TIMER_PERIOD * (motorsNumberOfSteps[Z] / (MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP)) * MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP));
		float denominator = ((motorsNumberOfSteps[Z] / (MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP)) * 60 * sysclk_get_cpu_hz()) / (CALIBRATING_Z_FEED_RATE * MOTORS_STEP_TIMER_PERIOD * (motorsNumberOfSteps[Z] / (MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP)) * MOTOR_Z_STEPS_PER_MM * MICROSTEPS_PER_STEP * motorsStepDelay[Z]) - 1;
		motorsDelaySkips[Z] = denominator ? round(1 / denominator) : 0;
		
		// Set motor Z Vref to active
		tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_CURRENT_ACTIVE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));
		
		// Turn on motors
		turnOn();
		
		// Get still value
		accelerometer.readAccelerationValues();
		int16_t stillValue = accelerometer.yAcceleration;
		
		// Start motors step timer
		startMotorsStepTimer();
	
		// Wait until Z motor step interrupts have stopped or an emergency stop occurs
		for(uint8_t counter = 0; MOTORS_STEP_TIMER.INTCTRLB & TC0_CCCINTLVL_gm && !emergencyStopOccured;) {
		
			// Get accelerometer values
			accelerometer.readAccelerationValues();
	
			// Check if extruder has hit the bed
			if(abs(stillValue - accelerometer.yAcceleration) >= Y_TILT_ACCELERATION) {
				if(++counter >= 2)
			
					// Stop motor Z interrupt
					tc_set_ccc_interrupt_level(&MOTORS_STEP_TIMER, TC_INT_LVL_OFF);
			}
			else
				counter = 0;
		}
	
		// Stop motors step timer
		stopMotorsStepTimer();
		
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
		
		// Move up by 2mm
		moveToHeight(currentValues[Z] + 2);
	}
	
	// Set motor Z Vref to idle
	tc_write_cc(&MOTORS_VREF_TIMER, MOTOR_Z_VREF_CHANNEL, round(MOTOR_Z_CURRENT_IDLE * MOTORS_CURRENT_TO_VOLTAGE_SCALAR / MICROCONTROLLER_VOLTAGE * MOTORS_VREF_TIMER_PERIOD));

	// Check if Z was previously valid and an emergency stop didn't happen
	if(validZ && !emergencyStopOccured)
	
		// Set that Z is valid
		currentStateOfValues[Z] = VALID;
}

void Motors::calibrateBedCenterZ0() {

	// Move up by 3mm
	moveToHeight(currentValues[Z] + 3);
	
	// Check if emergency stop hasn't occured
	if(!emergencyStopOccured) {

		// Home XY
		homeXY(false);
	
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
	float positionsX[] = {BED_CENTER_X - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_X + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_X + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_X - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_X};
	float positionsY[] = {BED_CENTER_Y - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y - BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y + BED_CALIBRATION_POSITIONS_DISTANCE_FROM_CENTER, BED_CENTER_Y};

	// Save mode
	MODES savedMode = mode;
	mode = ABSOLUTE;
	
	// Go through all positions
	for(uint8_t i = 0; i < sizeof(positionsX) / sizeof(positionsX[0]) && !emergencyStopOccured; i++) {

		// Move to position
		Gcode gcode;
		gcode.valueG = 0;
		gcode.valueX = positionsX[i];
		gcode.valueY = positionsY[i];
		gcode.valueF = EEPROM_SPEED_LIMIT_X_MAX;
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
		
		// Get position's orientation offset and length
		eeprom_addr_t eepromOffset = EEPROM_SIZE;
		uint8_t eepromLength;
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
			break;
			
			case 3:
				eepromOffset = EEPROM_BED_ORIENTATION_BACK_LEFT_OFFSET;
				eepromLength = EEPROM_BED_ORIENTATION_BACK_LEFT_LENGTH;
		}
		
		// Check if saving orientation
		if(eepromOffset < EEPROM_SIZE)
		
			// Save position's orientation
			nvm_eeprom_erase_and_write_buffer(eepromOffset, &currentValues[Z], eepromLength);
	
		// Move to height 3mm
		moveToHeight(3);
	}
	
	// Update bed changes
	updateBedChanges(false);
	
	// Restore mode
	mode = savedMode;
}

void Motors::reset() {

	// Stop motors step timer
	stopMotorsStepTimer();

	// Turn off motors
	turnOff();

	// Disable all motor step interrupts
	MOTORS_STEP_TIMER.INTCTRLB &= ~(TC0_CCAINTLVL_gm | TC0_CCBINTLVL_gm | TC0_CCCINTLVL_gm | TC0_CCDINTLVL_gm);
	
	// Clear emergency stop occured
	emergencyStopOccured = false;
}
