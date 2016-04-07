// Header gaurd
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H


// Accelerometer class
class Accelerometer {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Read Acceleration values
		void readAccelerationValues();
		
		// X, Y, and Z values
		int16_t xValue;
		int16_t yValue;
		int16_t zValue;
		
		// X, Y, and Z acceleration
		float xAcceleration;
		float yAcceleration;
		float zAcceleration;
		
		// Is working
		bool isWorking;
	
	// Private
	private:
		
		// Data available
		bool dataAvailable();
	
		// Send command
		void sendCommand(uint8_t command);
		
		// Write value
		void writeValue(uint8_t address, uint8_t value);
	
		// Read value
		void readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength = 1);
		
		// transmit
		void transmit(uint8_t command, uint8_t value = 0, bool sendValue = false, uint8_t *responseBuffer = NULL, uint8_t responseLength = 0);
};


#endif
