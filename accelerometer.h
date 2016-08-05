// Header gaurd
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H


// Definitions
#define Y_TILT_ACCELERATION 10


// Accelerometer class
class Accelerometer {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Has correct device ID
		bool hasCorrectDeviceId();
		
		// Read Acceleration values
		bool readAccelerationValues();
		
		// X, Y, and Z acceleration
		int16_t xAcceleration;
		int16_t yAcceleration;
		int16_t zAcceleration;
		
		// Is working
		bool isWorking;
	
	// Private
	private:
		
		// Data available
		inline bool dataAvailable();
	
		// Send command
		inline bool sendCommand(uint8_t command);
		
		// Write value
		inline bool writeValue(uint8_t address, uint8_t value);
	
		// Read value
		inline bool readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength = 1);
		
		// transmit
		inline bool transmit(uint8_t command, uint8_t value = 0, bool sendValue = false, uint8_t *responseBuffer = nullptr, uint8_t responseLength = 0);
};


#endif
