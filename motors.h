// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "accelerometer.h"
#include "gcode.h"
#include "vector.h"


// Definitions
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_VREF_TIMER_PERIOD 0x27F
#define MOTOR_E_CURRENT_SENSE_ADC ADCA

// Tasks
#define NO_TASK 0
#define BACKLASH_TASK 1
#define BED_LEVELING_TASK (1 << 1)
#define SAVE_CHANGES_TASK (1 << 2)

// Modes
enum MODES {RELATIVE, ABSOLUTE};

// Axes
enum AXES {X, Y, Z, E, F};

// Backlash direction
enum BACKLASH_DIRECTION {NONE, POSITIVE, NEGATIVE};


// Motors class
class Motors {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Turn on
		void turnOn();
		
		// Turn off
		void turnOff();
		
		// Move
		void move(const Gcode &gcode, uint8_t tasks = BACKLASH_TASK | BED_LEVELING_TASK | SAVE_CHANGES_TASK);
		
		// Home XY
		void homeXY();
		
		// Save Z as bed center Z0
		void saveZAsBedCenterZ0();
		
		// Calibrate bed center Z0
		void calibrateBedCenterZ0();
		
		// Calibrate bed center orientation
		void calibrateBedOrientation();
		
		// Emergency stop
		void emergencyStop();
		
		// Current values
		float currentValues[5];
		
		// Mode
		MODES mode;
		
		// Accelerometer
		Accelerometer accelerometer;
		
		// Emergency stop occured
		bool emergencyStopOccured;
	
	// Private
	private:
		
		// Move to height
		void moveToHeight(float height);
		
		// Compensate for backlash
		void compensateForBacklash(BACKLASH_DIRECTION backlashDirectionX, BACKLASH_DIRECTION backlashDirectionY);
		
		// Compensate for bed leveling
		void compensateForBedLeveling(float startValues[]);
	
		// Move to Z0
		void moveToZ0();
		
		// Get height adjustment required
		float getHeightAdjustmentRequired(float x, float y);
		
		// Current sense ADC controller and channel
		adc_config currentSenseAdcController;
		adc_channel_config currentSenseAdcChannel;
		
		// Bed height offset
		float bedHeightOffset;
		
		// Vectors
		Vector backRightVector;
		Vector backLeftVector;
		Vector frontLeftVector;
		Vector frontRightVector;
		Vector centerVector;
		Vector backPlane;
		Vector leftPlane;
		Vector rightPlane;
		Vector frontPlane;
};


#endif
