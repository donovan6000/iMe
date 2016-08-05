// MMA8652FC accelerometer http://cache.nxp.com/files/sensors/doc/data_sheet/MMA8652FC.pdf


// Header files
extern "C" {
	#include <asf.h>
}
#include "accelerometer.h"


// Definitions
#define ACCELEROMETER_ENABLE IOPORT_PIN_LEVEL_HIGH
#define ACCELEROMETER_DISABLE IOPORT_PIN_LEVEL_LOW
#define ACCELEROMETER_SAMPLE_SIZE 25

// Accelerometer pins
#define TWI_MASTER TWIC
#define ACCELEROMETER_ENABLE_PIN IOPORT_CREATE_PIN(PORTB, 1)
#define ACCELEROMETER_SDA_PIN IOPORT_CREATE_PIN(PORTC, 0)
#define ACCELEROMETER_SCL_PIN IOPORT_CREATE_PIN(PORTC, 1)

// Accelerometer settings
#define MASTER_ADDRESS 0x00
#define ACCELEROMETER_ADDRESS 0x1D
#define BUS_SPEED 400000
#define DEVICE_ID 0x4A

// Registers
#define STATUS 0x00
#define STATUS_XDR 0b00000001
#define STATUS_YDR 0b00000010
#define STATUS_ZDR 0b00000100
#define OUT_X_MSB 0x01
#define OUT_X_LSB 0x02
#define OUT_Y_MSB 0x03
#define OUT_Y_LSB 0x04
#define OUT_Z_MSB 0x05
#define OUT_Z_LSB 0x06
#define WHO_AM_I 0x0D
#define CTRL_REG1 0x2A
#define CTRL_REG1_ACTIVE 0b00000001
#define CTRL_REG1_DR0 0b00001000
#define CTRL_REG1_DR1 0b00010000
#define CTRL_REG1_DR2 0b00100000
#define CTRL_REG2 0x2B
#define CTRL_REG2_RST 0b01000000

// Accelerations
enum {ACCELERATION_X, ACCELERATION_Y, ACCELERATION_Z, NUMBER_OF_ACCELERATION_AXES};


// Supporting function implementation
void Accelerometer::initialize() {
	
	// Configure enable, SDA, and SCL pins
	ioport_set_pin_dir(ACCELEROMETER_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(ACCELEROMETER_ENABLE_PIN, ACCELEROMETER_ENABLE);
	ioport_set_pin_mode(ACCELEROMETER_SDA_PIN, IOPORT_MODE_WIREDANDPULL);
	ioport_set_pin_mode(ACCELEROMETER_SCL_PIN, IOPORT_MODE_WIREDANDPULL);
	
	// Configure interface
	twi_options_t options;
	options.speed = BUS_SPEED;
	options.chip = MASTER_ADDRESS;
	options.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), BUS_SPEED);
	
	// Initialize interface
	sysclk_enable_peripheral_clock(&TWI_MASTER);
	twi_master_init(&TWI_MASTER, &options);
	twi_master_enable(&TWI_MASTER);

	// Check if accelerometer has the correct device ID
	if(hasCorrectDeviceId()) {
		
		// Reset the accelerometer
		writeValue(CTRL_REG2, CTRL_REG2_RST);
		
		// Wait enough time for accelerometer to initialize
		delay_ms(2);
		
		// Put accelerometer into standby mode
		writeValue(CTRL_REG1, 0);
		
		// Set output data rate frequency to 400Hz and enable active mode
		writeValue(CTRL_REG1, CTRL_REG1_DR0 | CTRL_REG1_ACTIVE);
	}
}

bool Accelerometer::hasCorrectDeviceId() {

	// Return if accelerometer has the correct device ID
	uint8_t buffer;
	return isWorking = readValue(WHO_AM_I, &buffer) && buffer == DEVICE_ID;
}

bool Accelerometer::readAccelerationValues() {

	// Go through each axis
	int32_t averages[NUMBER_OF_ACCELERATION_AXES] = {};
	for(uint8_t i = 0; i < ACCELEROMETER_SAMPLE_SIZE; i++) {
		
		// Wait until data is available
		while(!dataAvailable())
		
			// Check if accelerometer isn't working
			if(!isWorking)
			
				// Break
				break;
	
		// Check if accelerometer isn't working or reading values failed
		uint8_t values[OUT_Z_LSB - OUT_X_MSB + 1];
		if(!isWorking || !readValue(OUT_X_MSB, values, OUT_Z_LSB - OUT_X_MSB + 1))
		
			// Break
			break;
		
		// Get acceleration
		for(uint8_t j = 0; j < NUMBER_OF_ACCELERATION_AXES; j++)
			averages[j] += ((values[j * 2] << 8) | values[j * 2 + 1]) >> 4;
	}
	
	// Get average acceleration
	for(uint8_t i = 0; i < NUMBER_OF_ACCELERATION_AXES; i++)
		averages[i] /= ACCELEROMETER_SAMPLE_SIZE;
	
	// Set acceleration values and account for accelerometer's orientation
	xAcceleration = averages[ACCELERATION_Z];
	yAcceleration = averages[ACCELERATION_Y];
	zAcceleration = averages[ACCELERATION_X];
	
	// Return if accelerometer is working
	return isWorking;
}

bool Accelerometer::dataAvailable() {

	// Return if data is available
	uint8_t buffer;
	readValue(STATUS, &buffer);
	return buffer & (STATUS_XDR | STATUS_YDR | STATUS_ZDR);
}

bool Accelerometer::sendCommand(uint8_t command) {

	// Return if sending command was successful
	return transmit(command);
}

bool Accelerometer::writeValue(uint8_t address, uint8_t value) {

	// Return if writing value was successful
	return transmit(address, value, true);
}

bool Accelerometer::readValue(uint8_t address, uint8_t *responseBuffer, uint8_t responseLength) {

	// Return if receiving response was successful
	return transmit(address, 0, false, responseBuffer, responseLength);
}

bool Accelerometer::transmit(uint8_t command, uint8_t value, bool sendValue, uint8_t *responseBuffer, uint8_t responseLength) {
	
	// Create packet
	twi_package_t packet;
	packet.addr[0] = command;
	if(sendValue) {
		packet.addr[1] = value;
		packet.addr_length = 2;
	}
	else
		packet.addr_length = 1;
	packet.chip = ACCELEROMETER_ADDRESS;
	packet.buffer = responseBuffer;
	packet.length = responseLength;
	packet.no_wait = false;
	
	// Return if transmission was successful
	return isWorking = twi_master_transfer(&TWI_MASTER, &packet, responseLength) == TWI_SUCCESS;
}
