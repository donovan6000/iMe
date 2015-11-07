// PORTE1 is Fan, active high
// PORTE3 is LED, active high

// Header files
#include <avr/eeprom.h>
#define F_CPU 24000000UL
#include <util/delay.h>

extern "C" {
	#include <asf.h>
}


// Definitions

// EEPROM offsets
#define EEPROM_BACKLASH_EXPANSION_E 0x42
#define EEPROM_BACKLASH_EXPANSION_X_PLUS 0x2E
#define EEPROM_BACKLASH_EXPANSION_YL_PLUS 0x32
#define EEPROM_BACKLASH_EXPANSION_YR_MINUS 0x3A
#define EEPROM_BACKLASH_EXPANSION_YR_PLUS 0x36
#define EEPROM_BACKLASH_EXPANSION_Z 0x3E
#define EEPROM_BACKLASH_SPEED 0x5E
#define EEPROM_BACKLASH_X 0x0C
#define EEPROM_BACKLASH_Y 0x10
#define EEPROM_BED_COMPENSATION_BACK_LEFT 0x18
#define EEPROM_BED_COMPENSATION_BACK_RIGHT 0x14
#define EEPROM_BED_COMPENSATION_FRONT_LEFT 0x1C
#define EEPROM_BED_COMPENSATION_FRONT_RIGHT 0x20
#define EEPROM_BED_COMPENSATION_VERSION 0x62
#define EEPROM_E_AXIS_STEPS_PER_MM 0x2E2
#define EEPROM_EXTRUDER_CURRENT 0x2E8
#define EEPROM_FAN_OFFSET 0x2AC
#define EEPROM_FAN_SCALE 0x2AD
#define EEPROM_FAN_TYPE 0x2AB
#define EEPROM_FILAMENT_AMOUNT 0x2A
#define EEPROM_FILAMENT_TEMPERATURE 0x29
#define EEPROM_FILAMENT_TYPE_ID 0x28
#define EEPROM_FIRMWARE_CRC 0x04
#define EEPROM_FIRMWARE_VERSION 0x00
#define EEPROM_HARDWARE_STATUS 0x2B8
#define EEPROM_HEATER_CALIBRATION_MODE 0x2B1
#define EEPROM_HEATER_RESISTANCE_M 0x2EA
#define EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B 0x2BA
#define EEPROM_HOURS_COUNTER_SPOOLER 0x2C0
#define EEPROM_LAST_RECORDED_Z_VALUE 0x08
#define EEPROM_SAVED_Z_STATE 0x2E6
#define EEPROM_SERIAL_NUMBER 0x2EF
#define EEPROM_SPOOL_RECORD_ID 0x24
#define EEPROM_X_AXIS_STEPS_PER_MM 0x2D6
#define EEPROM_X_MOTOR_CURRENT 0x2B2
#define EEPROM_Y_AXIS_STEPS_PER_MM 0x2DA
#define EEPROM_Y_MOTOR_CURRENT 0x2B4
#define EEPROM_Z_AXIS_STEPS_PER_MM 0x2DE
#define EEPROM_Z_CALIBRATION_BLO 0x46
#define EEPROM_Z_CALIBRATION_BRO 0x4A
#define EEPROM_Z_CALIBRATION_FLO 0x52
#define EEPROM_Z_CALIBRATION_FRO 0x4E
#define EEPROM_Z_CALIBRATION_ZO 0x56
#define EEPROM_Z_MOTOR_CURRENT 0x2B6


// Global variables
uint8_t serialNumber[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
uint8_t bufferSize = 0;
uint8_t buffer[64];


// Function prototypes

/*
Name: Set serial number
Purpose: Sets serial number used by the USB descriptor to the value in EEPROM
*/
void setSerialNumber(void);

/*
Name: CDC enable callback
Purpose: Callback for when USB is connected
*/
bool cdcEnableCallback(void);

/*
Name: CDC disable callback
Purpose: Callback for when USB is disconnected
*/
void cdcDisableCallback(void);

/*
Name: CDC RX notify callback
Purpose: Callback for when USB receives data
*/
void cdcRxNotifyCallback(uint8_t port);


// Main function
int main(void) {
	
	// Initialize system
	sysclk_init();
	board_init();
	
	// Set ports to values used by official firmware
	PORTA.DIR = 0x06;
	PORTA.PIN6CTRL = 0x18;

	PORTB.DIR = 0x0C;
	PORTB.OUT = 0x08;
	PORTB.PIN1CTRL = 0x18;

	PORTC.DIR = 0xFE;

	PORTD.DIR = 0x3F;
	PORTD.OUT = 0x30;

	PORTE.DIR = 0x0E;
	PORTE.PIN0CTRL = 0x18;
	
	// Set serial number
	setSerialNumber();
	
	// Enable interrupts
	irq_initialize_vectors();
	cpu_irq_enable();
	
	// Initialize USB
	udc_start();
	
	// Main loop
	while(1) {
	
		// Delay to allow enough time for buffer size to change
		_delay_us(1);
		
		// Check if data has been received
		if(bufferSize) {
		
			// Go through all data
			for(uint8_t i = 0; i < bufferSize; i++) {
			
				// Wait until USB transmitter is ready
				while(!udi_cdc_is_tx_ready());
				
				// Send data to USB
				udi_cdc_putc(buffer[i]);
			}
			
			// Clear buffer size
			bufferSize = 0;
		}
	}
	
	// Return 0
	return 0;
}


// Supporting function implementation
void setSerialNumber(void) {

	// Read serial from EEPROM
	eeprom_read_block((void *)&serialNumber, (const void *)EEPROM_SERIAL_NUMBER, 16);
}

bool cdcEnableCallback(void) {

	// Return true
	return true;
}

void cdcDisableCallback(void) {

}

void cdcRxNotifyCallback(uint8_t port) {

	// Get received data
	for(bufferSize = 0; udi_cdc_is_rx_ready(); bufferSize++)
		buffer[bufferSize] = udi_cdc_getc();
}
