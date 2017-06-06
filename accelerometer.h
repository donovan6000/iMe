// Header guard
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H


// Definitions
#define Y_TILT_ACCELERATION 10


// Accelerometer class
class Accelerometer {

	// Public
	public:
	
		// Initialize
		static void initialize();
		
		// Test connection
		static bool testConnection();
		
		// Read Acceleration values
		static bool readAccelerationValues();
		
		// X, Y, and Z acceleration
		static int16_t xAcceleration;
		static int16_t yAcceleration;
		static int16_t zAcceleration;
		
		// Is working
		static bool isWorking;
	
	// Private
	private:
		
		// Data available
		static inline bool dataAvailable();
	
		// Send command
		static bool sendCommand(uint8_t command);
		
		// Write value
		static bool writeValue(uint8_t address, uint8_t value);
	
		// Read value
		static bool readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength = 1);
		
		// transmit
		static bool transmit(uint8_t command, uint8_t value = 0, bool sendValue = false, uint8_t *responseBuffer = nullptr, uint8_t responseLength = 0);
};


#endif
