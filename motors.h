// Header guard
#ifndef MOTORS_H
#define MOTORS_H


// Header files
#include "accelerometer.h"
#include "gcode.h"
#include "vector.h"


// Definitions
#define MOTORS_VREF_TIMER TCD0
#define MOTORS_VREF_TIMER_PERIOD 0x27F
#define NUMBER_OF_MOTORS 4
#define MOTORS_SAVE_TIMER MOTORS_VREF_TIMER

// Tasks
#define NO_TASK 0
#define BACKLASH_TASK 1
#define BED_LEVELING_AND_SKEW_TASK (1 << 1)
#define HANDLE_RECEIVED_COMMAND_TASK (1 << 2)

// State changes
#define saveState(motor, parameter) changeState(true, motor, parameter)
#define restoreState() changeState()

// Modes
enum Modes {RELATIVE, ABSOLUTE};

// Axes
enum Axes {X, Y, Z, E, F};

// Backlash direction
enum BacklashDirection {NONE, POSITIVE, NEGATIVE};

// Axes parameter
enum AxesParameter {DIRECTION, VALIDITY, VALUE};

// Units
enum Units {MILLIMETERS, INCHES};

// Tiers
enum Tiers {LOW_TIER, MEDIUM_TIER, HIGH_TIER};


// Motors class
class Motors final {

	// Public
	public:
	
		// Initialize
		void initialize() noexcept;
		
		// Turn on
		void turnOn() noexcept;
		
		// Turn off
		void turnOff() noexcept;
		
		// Move
		bool move(const Gcode &gcode, uint8_t tasks = BACKLASH_TASK | BED_LEVELING_AND_SKEW_TASK | HANDLE_RECEIVED_COMMAND_TASK) noexcept;
		
		// Home XY
		bool homeXY(bool applyBedAndSkewCompensation = true) noexcept;
		
		// Save Z as bed center Z0
		void saveZAsBedCenterZ0() noexcept;
		
		// Calibrate bed center Z0
		bool calibrateBedCenterZ0(bool applyBedAndSkewCompensation = true) noexcept;
		
		// Calibrate bed center orientation
		bool calibrateBedOrientation() noexcept;
		
		// Update bed changes
		void updateBedChanges(bool affectValues = true) noexcept;
		
		// Gantry clips detected
		bool gantryClipsDetected() noexcept;
		
		// Change state
		void changeState(bool save = false, Axes motor = X, AxesParameter parameter = DIRECTION) noexcept;
		
		// Reset
		void reset() noexcept;
		
		// Is on
		bool isOn() noexcept;
		
		// State values
		float currentValues[5];
		bool currentMotorDirections[2];
		bool currentStateOfValues[3];
		
		// Modes
		Modes mode;
		Modes extruderMode;
		
		// Unit
		Units units;
		
		// Accelerometer
		Accelerometer accelerometer;
	
	// Private
	private:
		
		// Move to height
		void moveToHeight(float height, bool applyBedAndSkewCompensation = true) noexcept;
		
		// Check if backlash compensation is enabled
		#if ENABLE_BACKLASH_COMPENSATION == true
		
			// Compensate for backlash
			void compensateForBacklash(BacklashDirection backlashDirectionX, BacklashDirection backlashDirectionY) noexcept;
		#endif
		
		// Split up movement
		void splitUpMovement(bool applyBedAndSkewCompensation) noexcept;
	
		// Move to Z0
		bool moveToZ0() noexcept;
		
		// Check if bed leveling is enabled
		#if ENABLE_BED_LEVELING_COMPENSATION == true
		
			// Get height adjustment required
			float getHeightAdjustmentRequired(float x, float y) noexcept;
		#endif
		
		// Check if skew compensation is enabled
		#if ENABLE_SKEW_COMPENSATION == true
		
			// Get skew adjustment required
			float getSkewAdjustmentRequired(Axes axis, float height) noexcept;
		#endif
		
		// Start and stop motors step timer
		void startMotorsStepTimer() noexcept;
		inline void stopMotorsStepTimer() noexcept;
		
		// Are motors moving
		inline bool areMotorsMoving() noexcept;
		
		// Get movement's number of cycles
		inline float getMovementsNumberOfCycles(Axes motor, float stepsPerMm, float feedRate) noexcept;
		
		// Set motor delay and skip
		void setMotorDelayAndSkip(Axes motor, float movementsNumberOfCycles) noexcept;
		
		// Get tier at height
		inline Tiers getTierAtHeight(float height) noexcept;
		
		// Check iff regulating extruder's current
		#if REGULATE_EXTRUDER_CURRENT == true
		
			// Current sense ADC controller and channel
			adc_config currentSenseAdcController;
			adc_channel_config currentSenseAdcChannel;
		#endif
		
		// External bed height
		float externalBedHeight;
		
		// Check if skew compensation is enabled
		#if ENABLE_SKEW_COMPENSATION == true
		
			// Skew values
			float skewX, skewY;
		#endif
		
		// Segment start values
		float startValues[NUMBER_OF_MOTORS];
		
		// Check if bed leveling is enabled
		#if ENABLE_BED_LEVELING_COMPENSATION == true
		
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
		#endif
};


#endif
