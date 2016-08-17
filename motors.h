// Header gaurd
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "accelerometer.h"
#include "fan.h"
#include "gcode.h"
#include "vector.h"


// Definitions
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_VREF_TIMER_PERIOD 0x27F
#define NUMBER_OF_MOTORS 4
#define MOTORS_SAVE_TIMER FAN_TIMER
//#define REGULATE_EXTRUDER_CURRENT

// Tasks
#define NO_TASK 0
#define BACKLASH_TASK 1
#define BED_LEVELING_TASK (1 << 1)

// State changes
#define saveState(motor, parameter) changeState(true, motor, parameter)
#define restoreState() changeState()

// Modes
enum MODES {RELATIVE, ABSOLUTE};

// Axes
enum AXES {X, Y, Z, E, F};

// Backlash direction
enum BACKLASH_DIRECTION {NONE, POSITIVE, NEGATIVE};

// Axes parameter
enum AXES_PARAMETER {DIRECTION, VALIDITY, VALUE};


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
		bool move(const Gcode &gcode, uint8_t tasks = BACKLASH_TASK | BED_LEVELING_TASK);
		
		// Home XY
		bool homeXY(bool adjustHeight = true);
		
		// Save Z as bed center Z0
		void saveZAsBedCenterZ0();
		
		// Calibrate bed center Z0
		bool calibrateBedCenterZ0();
		
		// Calibrate bed center orientation
		bool calibrateBedOrientation();
		
		// Update bed changes
		void updateBedChanges(bool adjustHeight = true);
		
		// Gantry clips detected
		bool gantryClipsDetected();
		
		// Reset
		void reset();
		
		// State values
		float currentValues[5];
		bool currentMotorDirections[2];
		bool currentStateOfValues[3];
		
		// Mode
		MODES mode;
		
		// Accelerometer
		Accelerometer accelerometer;
		
		// Emergency stop occured
		bool emergencyStopOccured;
	
	// Private
	private:
	
		// Change state
		void changeState(bool save = false, AXES motor = X, AXES_PARAMETER parameter = DIRECTION);
		
		// Move to height
		void moveToHeight(float height);
		
		// Compensate for backlash
		void compensateForBacklash(BACKLASH_DIRECTION backlashDirectionX, BACKLASH_DIRECTION backlashDirectionY);
		
		// Split up movement
		void splitUpMovement(bool adjustHeight);
	
		// Move to Z0
		bool moveToZ0();
		
		// Get height adjustment required
		float getHeightAdjustmentRequired(float x, float y);
		
		// Start and stop motors step timer
		void startMotorsStepTimer();
		void stopMotorsStepTimer();
		
		// Are motors moving
		bool areMotorsMoving();
		
		// Current sense ADC controller and channel
		#ifdef REGULATE_EXTRUDER_CURRENT
			adc_config currentSenseAdcController;
			adc_channel_config currentSenseAdcChannel;
		#endif
		
		// Segment start values
		float startValues[NUMBER_OF_MOTORS];
		
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
