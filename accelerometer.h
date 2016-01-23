// Header gaurd
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H


// Header files
#include <stdint.h>


// Accelerometer class
class Accelerometer {

	// Public
	public :
	
		// Parameterized constructor
		Accelerometer();
		
		// Calibrate
		void calibrate();
		
		// Read Acceleration values
		void readAccelerationValues();
		
		// X, Y, and Z acceleration
		int32_t xAcceleration;
		int32_t yAcceleration;
		int32_t zAcceleration;
		
		// Is working
		bool isWorking;
	
	// Private
	private :
		
		// Initialize settings
		void initializeSettings();
	
		// Send command
		void sendCommand(uint8_t command);
		
		// Write value
		void writeValue(uint8_t address, uint8_t value);
		
		// Data available
		bool dataAvailable();
	
		// Read value
		void readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength = 1);
		
		// transmit
		void transmit(uint8_t command, uint8_t value = 0, bool sendValue = false, uint8_t *responseBuffer = NULL, uint8_t responseLength = 0);
		
		// X, Y, and Z values
		int16_t xValue;
		int16_t yValue;
		int16_t zValue;
};


#endif
