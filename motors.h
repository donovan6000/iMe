// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "accelerometer.h"
#include "gcode.h"


// Definitions
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_VREF_TIMER_PERIOD 0x27F
#define MOTOR_E_CURRENT_SENSE_ADC ADCA

// Modes
enum MODES {RELATIVE, ABSOLUTE};

// Steps
enum STEPS {STEP8 = 8, STEP16 = 16, STEP32 = 32};

// Axes
enum AXES {X, Y, Z, E, F};


// Motors class
class Motors {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Set micro steps per step
		void setMicroStepsPerStep(STEPS step);
		
		// Turn on
		void turnOn();
		
		// Turn off
		void turnOff();
		
		// Move
		void move(const Gcode &command);
		
		// Go home
		void goHome();
		
		// Set Z to zero
		void setZToZero();
		
		// Emergency stop
		void emergencyStop();
		
		// Current values
		float currentValues[5];
		
		// Mode
		MODES mode;
		
		// Accelerometer
		Accelerometer accelerometer;
	
	// Private
	private:
		
		// Steps
		STEPS step;
		
		// Emergency stop occured
		bool emergencyStopOccured = false;
		
		// Current sense ADC channel
		adc_channel_config currentSenseAdcChannel;
};


#endif
