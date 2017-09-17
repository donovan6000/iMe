// Header guard
#ifndef ACCELEROMETER_H
#define ACCELEROMETER_H


// Definitions
#define Y_TILT_ACCELERATION 10

// Accelerations
enum Accelerations : uint8_t {ACCELERATION_Z, ACCELERATION_Y, ACCELERATION_X, NUMBER_OF_ACCELERATION_AXES};


// Accelerometer class
class Accelerometer final {

	// Public
	public:
	
		// Initialize
		static void initialize() noexcept;
		
		// Test connection
		static bool testConnection() noexcept;
		
		// Read Acceleration values
		static bool readAccelerationValues() noexcept;
		
		// X, Y, and Z acceleration
		static int16_t accelerations[NUMBER_OF_ACCELERATION_AXES];
		
		// Is working
		static bool isWorking;
	
	// Private
	private:
		
		// Data available
		static inline bool dataAvailable() noexcept;
	
		// Send command
		static bool sendCommand(uint8_t command) noexcept;
		
		// Write value
		static bool writeValue(uint8_t address, uint8_t value) noexcept;
	
		// Read value
		static inline bool readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength = 1) noexcept;
		
		// transmit
		static bool transmit(uint8_t command, uint8_t value = 0, bool sendValue = false, uint8_t *responseBuffer = nullptr, uint8_t responseLength = 0) noexcept;
};


#endif
