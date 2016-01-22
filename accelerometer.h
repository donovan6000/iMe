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
		Accelerometer(uint8_t masterAddress, uint8_t slaveAddress, uint32_t speed);
		
		// Get X, Y, and Z
		uint16_t getX();
		uint16_t getY();
		uint16_t getZ();
		
		// Is working
		bool isWorking;
	
	// Private
	private :
	
		// Write
		void write(uint8_t command);
		
		// Write register
		void writeRegister(uint8_t address, uint8_t value);
	
		// Read
		uint8_t read(uint8_t address);
		
		// transmit
		uint8_t transmit(uint8_t command, bool writeValue = false, uint8_t value = 0, bool returnResponse = false);
		
		// Slave address
		uint8_t slaveAddress;
};


#endif
