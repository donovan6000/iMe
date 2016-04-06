// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "accelerometer.h"
#include "gcode.h"


// Definitions
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_VREF_TIMER_PERIOD 0x27F

enum MODES {RELATIVE, ABSOLUTE};
enum STEPS {STEP8 = 8, STEP16 = 16, STEP32 = 32};
enum AXES {X, Y, Z, E, F, E_POSITIVE = 3, E_NEGATIVE = 4};


// Motors class
class Motors {

	// Public
	public :
	
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
	private :
		
		// Steps
		STEPS step;
		
		// Speed limits
		float motorsSpeedLimit[5];
		
		// Emergency stop occured
		bool emergencyStopOccured = false;
		
		// Gcode
		Gcode gcode;
};


#endif
