// PORTE1 is Fan, active high
// PORTE3 is LED, active high

// Header files
#include <avr/eeprom.h>
#define F_CPU 24000000UL
#include <util/delay.h>
#include <string.h>

extern "C" {
	#include <asf.h>
}

#include "gcode.h"


// Definitions

#ifndef VERSION
	#define VERSION "1900000001"
#endif

// EEPROM offsets
#define EEPROM_FIRMWARE_VERSION_OFFSET 0x00
#define EEPROM_FIRMWARE_VERSION_LENGTH 4
#define EEPROM_FIRMWARE_CRC_OFFSET 0x04
#define EEPROM_FIRMWARE_CRC_LENGTH 4
#define EEPROM_LAST_RECORDED_Z_VALUE_OFFSET 0x08
#define EEPROM_LAST_RECORDED_Z_VALUE_LENGTH 4
#define EEPROM_BACKLASH_X_OFFSET 0x0C
#define EEPROM_BACKLASH_X_LENGTH 4
#define EEPROM_BACKLASH_Y_OFFSET 0x10
#define EEPROM_BACKLASH_Y_LENGTH 4
#define EEPROM_BED_ORIENTATION_BACK_RIGHT_OFFSET 0x14
#define EEPROM_BED_ORIENTATION_BACK_RIGHT_LENGTH 4
#define EEPROM_BED_ORIENTATION_BACK_LEFT_OFFSET 0x18
#define EEPROM_BED_ORIENTATION_BACK_LEFT_LENGTH 4
#define EEPROM_BED_ORIENTATION_FRONT_LEFT_OFFSET 0x1C
#define EEPROM_BED_ORIENTATION_FRONT_LEFT_LENGTH 4
#define EEPROM_BED_ORIENTATION_FRONT_RIGHT_OFFSET 0x20
#define EEPROM_BED_ORIENTATION_FRONT_RIGHT_LENGTH 4
#define EEPROM_FILAMENT_COLOR_OFFSET 0x24
#define EEPROM_FILAMENT_COLOR_LENGTH 4
#define EEPROM_FILAMENT_TYPE_AND_LOCATION_OFFSET 0x28
#define EEPROM_FILAMENT_TYPE_AND_LOCATION_LENGTH 1
#define EEPROM_FILAMENT_TEMPERATURE_OFFSET 0x29
#define EEPROM_FILAMENT_TEMPERATURE_LENGTH 1
#define EEPROM_FILAMENT_AMOUNT_OFFSET 0x2A
#define EEPROM_FILAMENT_AMOUNT_LENGTH 4
#define EEPROM_BACKLASH_EXPANSION_X_PLUS_OFFSET 0x2E
#define EEPROM_BACKLASH_EXPANSION_X_PLUS_LENGTH 4
#define EEPROM_BACKLASH_EXPANSION_Y_L_PLUS_OFFSET 0x32
#define EEPROM_BACKLASH_EXPANSION_Y_L_PLUS_LENGTH 4
#define EEPROM_BACKLASH_EXPANSION_Y_R_PLUS_OFFSET 0x36
#define EEPROM_BACKLASH_EXPANSION_Y_R_PLUS_LENGTH 4
#define EEPROM_BACKLASH_EXPANSION_Y_R_MINUS_OFFSET 0x3A
#define EEPROM_BACKLASH_EXPANSION_Y_R_MINUS_LENGTH 4
#define EEPROM_BACKLASH_EXPANSION_Z_OFFSET 0x3E
#define EEPROM_BACKLASH_EXPANSION_Z_LENGTH 4
#define EEPROM_BACKLASH_EXPANSION_E_OFFSET 0x42
#define EEPROM_BACKLASH_EXPANSION_E_LENGTH 4
#define EEPROM_BED_OFFSET_BACK_LEFT_OFFSET 0x46
#define EEPROM_BED_OFFSET_BACK_LEFT_LENGTH 4
#define EEPROM_BED_OFFSET_BACK_RIGHT_OFFSET 0x4A
#define EEPROM_BED_OFFSET_BACK_RIGHT_LENGTH 4
#define EEPROM_BED_OFFSET_FRONT_RIGHT_OFFSET 0x4E
#define EEPROM_BED_OFFSET_FRONT_RIGHT_LENGTH 4
#define EEPROM_BED_OFFSET_FRONT_LEFT_OFFSET 0x52
#define EEPROM_BED_OFFSET_FRONT_LEFT_LENGTH 4
#define EEPROM_BED_HEIGHT_OFFSET_OFFSET 0x56
#define EEPROM_BED_HEIGHT_OFFSET_LENGTH 4
#define EEPROM_RESERVED_OFFSET 0x5A
#define EEPROM_RESERVED_LENGTH 4
#define EEPROM_BACKLASH_SPEED_OFFSET 0x5E
#define EEPROM_BACKLASH_SPEED_LENGTH 4
#define EEPROM_BED_ORIENTATION_VERSION_OFFSET 0x62
#define EEPROM_BED_ORIENTATION_VERSION_LENGTH 1
#define EEPROM_SPEED_LIMIT_X_OFFSET 0x66
#define EEPROM_SPEED_LIMIT_X_LENGTH 4
#define EEPROM_SPEED_LIMIT_Y_OFFSET 0x6A
#define EEPROM_SPEED_LIMIT_Y_LENGTH 4
#define EEPROM_SPEED_LIMIT_Z_OFFSET 0x6E
#define EEPROM_SPEED_LIMIT_Z_LENGTH 4
#define EEPROM_SPEED_LIMIT_E_POSITIVE_OFFSET 0x72
#define EEPROM_SPEED_LIMIT_E_POSITIVE_LENGTH 4
#define EEPROM_SPEED_LIMIT_E_NEGATIVE_OFFSET 0x76
#define EEPROM_SPEED_LIMIT_E_NEGATIVE_LENGTH 4
#define EEPROM_BED_ORIENTATION_FIRST_SAMPLE_OFFSET 0x106
#define EEPROM_BED_ORIENTATION_FIRST_SAMPLE_LENGTH 4
#define EEPROM_FAN_TYPE_OFFSET 0x2AB
#define EEPROM_FAN_TYPE_LENGTH 1
#define EEPROM_FAN_OFFSET_OFFSET 0x2AC
#define EEPROM_FAN_OFFSET_LENGTH 1
#define EEPROM_FAN_SCALE_OFFSET 0x2AD
#define EEPROM_FAN_SCALE_LENGTH 4
#define EEPROM_HEATER_CALIBRATION_MODE_OFFSET 0x2B1
#define EEPROM_HEATER_CALIBRATION_MODE_LENGTH 1
#define EEPROM_X_MOTOR_CURRENT_OFFSET 0x2B2
#define EEPROM_X_MOTOR_CURRENT_LENGTH 2
#define EEPROM_Y_MOTOR_CURRENT_OFFSET 0x2B4
#define EEPROM_Y_MOTOR_CURRENT_LENGTH 2
#define EEPROM_Z_MOTOR_CURRENT_OFFSET 0x2B6
#define EEPROM_Z_MOTOR_CURRENT_LENGTH 2
#define EEPROM_HARDWARE_STATUS_OFFSET 0x2B8
#define EEPROM_HARDWARE_STATUS_LENGTH 2
#define EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_OFFSET 0x2BA
#define EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_LENGTH 4
#define EEPROM_HOURS_COUNTER_OFFSET 0x2C0
#define EEPROM_HOURS_COUNTER_LENGTH 4
#define EEPROM_X_AXIS_STEPS_PER_MM_OFFSET 0x2D6
#define EEPROM_X_AXIS_STEPS_PER_MM_LENGTH 4
#define EEPROM_Y_AXIS_STEPS_PER_MM_OFFSET 0x2DA
#define EEPROM_Y_AXIS_STEPS_PER_MM_LENGTH 4
#define EEPROM_Z_AXIS_STEPS_PER_MM_OFFSET 0x2DE
#define EEPROM_Z_AXIS_STEPS_PER_MM_LENGTH 4
#define EEPROM_E_AXIS_STEPS_PER_MM_OFFSET 0x2E2
#define EEPROM_E_AXIS_STEPS_PER_MM_LENGTH 4
#define EEPROM_SAVED_Z_STATE_OFFSET 0x2E6
#define EEPROM_SAVED_Z_STATE_LENGTH 2
#define EEPROM_EXTRUDER_CURRENT_OFFSET 0x2E8
#define EEPROM_EXTRUDER_CURRENT_LENGTH 2
#define EEPROM_HEATER_RESISTANCE_M_OFFSET 0x2EA
#define EEPROM_HEATER_RESISTANCE_M_LENGTH 4
#define EEPROM_SERIAL_NUMBER_OFFSET 0x2EF
#define EEPROM_SERIAL_NUMBER_LENGTH 16


// Global variables
uint8_t serialNumber[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
uint8_t requestBufferSize = 0;
uint8_t requestBuffer[255];
char responseBuffer[255];


// Function prototypes

/*
Name: Set serial number
Purpose: Sets serial number used by the USB descriptor to the value in the EEPROM
*/
void setSerialNumber();

/*
Name: CDC enable callback
Purpose: Callback for when USB is connected
*/
bool cdcEnableCallback();

/*
Name: CDC disable callback
Purpose: Callback for when USB is disconnected
*/
void cdcDisableCallback();

/*
Name: CDC RX notify callback
Purpose: Callback for when USB receives data
*/
void cdcRxNotifyCallback(uint8_t port);


// Main function
int main() {

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
	
	// Initialize variables
	Gcode gcode;
	
	// Main loop
	while(1) {
	
		// Delay to allow enough time for a response to be received
		_delay_us(1);
		
		// Check if a request has been received
		if(requestBufferSize) {
		
			// Parse command
			gcode.parseCommand(reinterpret_cast<char*>(requestBuffer));
		
			// Clear request buffer size
			requestBufferSize = 0;
		
			// Check if command contains valid G-code
			if(!gcode.isEmpty()) {
			
				// Check if command is to reset
				if(gcode.getParameterM() == 115 && gcode.getParameterS() == 628) {
		
					// Trigger software reset
					CPU_CCP = CCP_IOREG_gc;
					RST.CTRL = RST_SWRST_bm;
				}
		
				// Otherwise check if command is requesting device details
				else if(gcode.getParameterM() == 115) {
		
					// Put device details into response
					strcpy(responseBuffer, "ok REPRAP_PROTOCOL:1 FIRMWARE_NAME:iMe FIRMWARE_VERSION:" VERSION " MACHINE_TYPE:The_Micro X-SERIAL_NUMBER:");
					strncat(responseBuffer, reinterpret_cast<char*>(serialNumber), EEPROM_SERIAL_NUMBER_LENGTH);
					strcat(responseBuffer, "\n");
				}
		
				// Otherwise check if command is requesting temperature
				else if(gcode.getParameterM() == 105)
		
					// Put temperature into response
					strcpy(responseBuffer, "ok T:0\n");
		
				// Otherwise
				else
		
					// Put confirmation into response
					strcpy(responseBuffer, "ok\n");
			}
			
			// Otherwise
			else
			
				// Put error into response
				strcpy(responseBuffer, "ok Error: Invalid command\n");
			
			// Send response
			udi_cdc_write_buf(responseBuffer, strlen(responseBuffer));
		}
	}
	
	// Return 0
	return 0;
}


// Supporting function implementation
void setSerialNumber() {

	// Read serial from EEPROM
	eeprom_read_block(&serialNumber, reinterpret_cast<void *>(EEPROM_SERIAL_NUMBER_OFFSET), EEPROM_SERIAL_NUMBER_LENGTH);
}

bool cdcEnableCallback() {

	// Return true
	return true;
}

void cdcDisableCallback() {

}

void cdcRxNotifyCallback(uint8_t port) {

	// Check if last request as been processed
	if(!requestBufferSize) {
	
		// Get request
		requestBufferSize = udi_cdc_get_nb_received_data();
		udi_cdc_read_buf(requestBuffer, requestBufferSize);
		requestBuffer[requestBufferSize] = 0;
	}
}
