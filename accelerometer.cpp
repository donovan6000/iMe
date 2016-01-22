// Header files
extern "C" {
	#include <asf.h>
}
#include "accelerometer.h"


// Definitions

// Pins
#define ACCELEROMETER_VDDIO IOPORT_CREATE_PIN(PORTB, 1)
#define TWI_MASTER TWIC
#define TWI_MASTER_PORT PORTC

// Registers
#define OUT_X_MSB 0x01
#define OUT_X_LSB 0x02
#define OUT_Y_MSB 0x03
#define OUT_Y_LSB 0x04
#define OUT_Z_MSB 0x05
#define OUT_Z_LSB 0x06
#define WHO_AM_I 0x0D
#define CTRL_REG1 0x2A


// Supporting function implementation
Accelerometer::Accelerometer(uint8_t masterAddress, uint8_t slaveAddress, uint32_t speed) {

	// Initialize variables
	uint8_t value;
	
	// Save slave address
	this->slaveAddress = slaveAddress;
	
	// Enable accelerometer
	ioport_set_pin_dir(ACCELEROMETER_VDDIO, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(ACCELEROMETER_VDDIO, IOPORT_PIN_LEVEL_HIGH);
	
	// Configure SDA and SCL pins
	TWI_MASTER_PORT.PIN0CTRL = PORT_OPC_WIREDANDPULL_gc;
	TWI_MASTER_PORT.PIN1CTRL = PORT_OPC_WIREDANDPULL_gc;
	
	// Configure interface
	twi_options_t options;
	options.speed = speed;
	options.chip = masterAddress;
	options.speed_reg = TWI_BAUD(sysclk_get_cpu_hz(), speed);
	
	// Initialize interface
	sysclk_enable_peripheral_clock(&TWI_MASTER);
	twi_master_init(&TWI_MASTER, &options);
	twi_master_enable(&TWI_MASTER);

	// Create packet
	twi_package_t packet;
	packet.addr[0] = WHO_AM_I;
	packet.addr_length = 1;
	packet.chip = this->slaveAddress;
	packet.buffer = &value;
	packet.length = 1;
	packet.no_wait = false;
	
	// Check if transmitting or receiving failed
	if(twi_master_read(&TWI_MASTER, &packet) != TWI_SUCCESS || value != 0x4A)
	
		// Clear is working
		isWorking = false;
	
	// Otherwise
	else {
	
		// Set is working to if accelerometer was successfully put into active mode
		writeRegister(CTRL_REG1, 0x01);
		isWorking = read(CTRL_REG1) == 0x01;
	}
}

uint16_t Accelerometer::getX() {

	// Return X value
	return (read(OUT_X_MSB) << 8) | read(OUT_X_LSB);
}

uint16_t Accelerometer::getY() {

	// Return Y value
	return (read(OUT_Y_MSB) << 8) | read(OUT_Y_LSB);
}

uint16_t Accelerometer::getZ() {

	// Return Z value
	return (read(OUT_Z_MSB) << 8) | read(OUT_Z_LSB);
}

void Accelerometer::write(uint8_t command) {

	// Transmit request
	transmit(command);
}

void Accelerometer::writeRegister(uint8_t address, uint8_t value) {

	// Transmit request
	transmit(address, true, value);
}

uint8_t Accelerometer::read(uint8_t address) {

	// Return response
	return transmit(address, false, 0, true);
}

uint8_t Accelerometer::transmit(uint8_t command, bool writeValue, uint8_t value, bool returnResponse) {

	// Initialize variables
	uint8_t response;
	
	// Create packet
	twi_package_t packet;
	
	packet.addr[0] = command;
	
	if(writeValue) {
		packet.addr[1] = value;
		packet.addr_length = 2;
	}
	else
		packet.addr_length = 1;
	
	packet.chip = this->slaveAddress;
	packet.buffer = &response;
	packet.length = returnResponse ? 1 : 0;
	packet.no_wait = false;
	
	// Wait until transmission is done
	while(twi_master_transfer(&TWI_MASTER, &packet, returnResponse) != TWI_SUCCESS);
	
	// Return response
	return response;
}
