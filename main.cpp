// ATxmega32C4 microcontroller http://www.atmel.com/Images/Atmel-8493-8-and-32-bit-AVR-XMEGA-Microcontrollers-ATxmega16C4-ATxmega32C4_Datasheet.pdf
// 5V 4A 2.1mmx5.5mm DC power supply
// USB type B connection


// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include "common.h"
#include "eeprom.h"
#include "fan.h"
#include "gcode.h"
#include "heater.h"
#include "led.h"
#include "motors.h"


// Definitions
#define REQUEST_BUFFER_SIZE 5
#define WAIT_TIMER MOTORS_VREF_TIMER
#define WAIT_TIMER_PERIOD MOTORS_VREF_TIMER_PERIOD
#define ALLOW_USELESS_COMMANDS false

// Unknown pin (Connected to transistors above the microcontroller. Maybe related to detecting if USB is connected)
#define UNKNOWN_PIN IOPORT_CREATE_PIN(PORTA, 1)

// Unused pins (None of them are connected to anything, so they could be used to easily interface additional hardware to the printer)
#define UNUSED_PIN_1 IOPORT_CREATE_PIN(PORTA, 6)
#define UNUSED_PIN_2 IOPORT_CREATE_PIN(PORTB, 0)
#define UNUSED_PIN_3 IOPORT_CREATE_PIN(PORTE, 0)
#define UNUSED_PIN_4 IOPORT_CREATE_PIN(PORTR, 0)
#define UNUSED_PIN_5 IOPORT_CREATE_PIN(PORTR, 1)


// Global variables
char serialNumber[EEPROM_SERIAL_NUMBER_LENGTH];
Gcode requests[REQUEST_BUFFER_SIZE];
uint16_t waitTimerCounter;
bool emergencyStopOccured = false;
Heater heater;
Motors motors;


// Function prototypes

/*
Name: CDC RX notify callback
Purpose: Callback for when USB receives data
*/
void cdcRxNotifyCallback(uint8_t port);

/*
Name: CDC disconnect callback
Purpose: Callback for when USB is disconnected from host (This should get called when the device is disconnected, but it gets called when the device is reattached which makes it necessary to guarantee that all data being sent over USB can actually be sent to prevent a buffer from overflowing when the device is disconnected. This is either a silicon error or an error in Atmel's ASF library)
*/
void cdcDisconnectCallback(uint8_t port);

/*
Name: Disable sending wait responses
Purpose: Disables sending wait responses every second
*/
void disableSendingWaitResponses();

/*
Name: Enable sending wait responses
Purpose: Enabled sending wait responses every second
*/
void enableSendingWaitResponses();


// Main function
int main() {
	
	// Initialize system clock
	sysclk_init();
	
	// Initialize interrupt controller
	pmic_init();
	pmic_set_scheduling(PMIC_SCH_ROUND_ROBIN);
	
	// Initialize board
	board_init();
	
	// Initialize I/O ports
	ioport_init();
	
	// Initialize requests
	for(uint8_t i = 0; i < REQUEST_BUFFER_SIZE; i++)
		requests[i].isParsed = false;
	
	// Initialize variables
	Fan fan;
	Led led;
	uint64_t currentCommandNumber = 0;
	uint8_t currentProcessingRequest = 0;
	char responseBuffer[UINT8_MAX + 1];
	char numberBuffer[INT_BUFFER_SIZE];
	
	// Configure ADC Vref pin
	ioport_set_pin_dir(ADC_VREF_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(ADC_VREF_PIN, IOPORT_MODE_PULLDOWN);
	
	// Enable ADC module
	adc_enable(&ADC_MODULE);
	
	// Initialize peripherals
	fan.initialize();
	heater.initialize();
	led.initialize();
	motors.initialize();
	
	// Configure unknown pin
	ioport_set_pin_dir(UNKNOWN_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(UNKNOWN_PIN, IOPORT_PIN_LEVEL_LOW);
	
	// Configure unused pins
	ioport_set_pin_dir(UNUSED_PIN_1, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(UNUSED_PIN_1, IOPORT_MODE_PULLUP);
	ioport_set_pin_dir(UNUSED_PIN_2, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(UNUSED_PIN_2, IOPORT_MODE_PULLUP);
	ioport_set_pin_dir(UNUSED_PIN_3, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(UNUSED_PIN_3, IOPORT_MODE_PULLUP);
	ioport_set_pin_dir(UNUSED_PIN_4, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(UNUSED_PIN_4, IOPORT_MODE_PULLUP);
	ioport_set_pin_dir(UNUSED_PIN_5, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(UNUSED_PIN_5, IOPORT_MODE_PULLUP);
	
	// Configure send wait interrupt
	tc_set_overflow_interrupt_callback(&WAIT_TIMER, []() -> void {
	
		// Check if one second has passed
		if(++waitTimerCounter >= sysclk_get_cpu_hz() / WAIT_TIMER_PERIOD) {
		
			// Reset wait timer counter
			waitTimerCounter = 0;
			
			// Send wait
			sendDataToUsb("wait\n", true);
		}
	});
	
	// Fix writing to EEPROM addresses above 0x2E0 by first writing to an address less than that (This is either a silicon error or an error in Atmel's ASF library)
	nvm_eeprom_write_byte(0, nvm_eeprom_read_byte(0));
	
	// Read serial number from EEPROM
	nvm_eeprom_write_byte(EEPROM_SERIAL_NUMBER_OFFSET + EEPROM_SERIAL_NUMBER_LENGTH - 1, 0);
	nvm_eeprom_read_buffer(EEPROM_SERIAL_NUMBER_OFFSET, serialNumber, EEPROM_SERIAL_NUMBER_LENGTH);
	
	// Enable interrupts
	cpu_irq_enable();
	
	// Initialize USB
	udc_start();
	
	// Enable sending wait responses
	enableSendingWaitResponses();
	
	// Main loop
	while(true) {
	
		// Check if a current processing request is ready
		if(requests[currentProcessingRequest].isParsed) {
		
			// Disable sending wait responses
			disableSendingWaitResponses();
			
			// Check if an emergency stop hasn't occured
			if(!emergencyStopOccured) {
		
				// Check if accelerometer isn't working
				if(!motors.accelerometer.isWorking && !motors.accelerometer.testConnection())
				
					// Set response to error
					strcpy(responseBuffer, "Error: Accelerometer isn't working");
				
				// Check if heater isn't working
				else if(!heater.isWorking && !heater.testConnection())
				
					// Set response to error
					strcpy(responseBuffer, "Error: Heater isn't working");
				
				// Otherwise
				else {
				
					// Clear response buffer
					*responseBuffer = 0;
	
					// Check if host commands are allowed
					#if ALLOW_HOST_COMMANDS == true

						// Check if command is a host command
						if(requests[currentProcessingRequest].commandParameters & PARAMETER_HOST_COMMAND_OFFSET) {
						
							// Check if host command is to get lock bits
							if(!strcmp(requests[currentProcessingRequest].hostCommand, "Lock bits")) {
					
								// Send lock bits
								strcpy(responseBuffer, "ok\n0x");
								ltoa(NVM.LOCK_BITS, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
					
							// Otherwise check if host command is to get fuse bytes
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Fuse bytes")) {
					
								// Send fuse bytes
								strcpy(responseBuffer, "ok\n0x");
								ltoa(nvm_fuses_read(FUSEBYTE0), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(nvm_fuses_read(FUSEBYTE1), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(nvm_fuses_read(FUSEBYTE2), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(nvm_fuses_read(FUSEBYTE3), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(nvm_fuses_read(FUSEBYTE4), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(nvm_fuses_read(FUSEBYTE5), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
					
							// Otherwise check if host command is to get application contents
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application contents")) {
					
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
					
								// Send application
								for(uint16_t i = APP_SECTION_START; i <= APP_SECTION_END; i++) {
									strcpy(responseBuffer, i == APP_SECTION_START ? "0x" : " 0x");
									ltoa(pgm_read_byte(i), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									if(i != APP_SECTION_END)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get application table contents
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application table contents")) {
					
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
					
								// Send application
								for(uint16_t i = APPTABLE_SECTION_START; i <= APPTABLE_SECTION_END; i++) {
									strcpy(responseBuffer, i == APPTABLE_SECTION_START ? "0x" : " 0x");
									ltoa(pgm_read_byte(i), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									if(i != APPTABLE_SECTION_END)
										sendDataToUsb(responseBuffer);
								}
							}
					
							// Otherwise check if host command is to get bootloader contents
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Bootloader contents")) {
					
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
								
								// Send bootloader
								for(uint16_t i = BOOT_SECTION_START; i <= BOOT_SECTION_END; i++) {
									strcpy(responseBuffer, i == BOOT_SECTION_START ? "0x" : " 0x");
									ltoa(pgm_read_byte(i), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									if(i != BOOT_SECTION_END)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get bootloader CRC steps
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Bootloader CRC steps")) {
							
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
							
								// Go through every other byte in the bootloader
								for(uint16_t i = 1; i < BOOT_SECTION_SIZE; i += 2) {
							
									// Wait until non-volatile memory controller isn't busy
									nvm_wait_until_ready();
				
									// Set CRC to use 0xFFFFFFFF seed and target flash memory
									CRC.CTRL = CRC_RESET_RESET1_gc;
									CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;
							
									// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise. This is an error in Atmel's ASF library)
									#if FLASH_SIZE < 0x10000UL
										NVM.ADDR2 = 0;
										NVM.DATA2 = 0;
									#endif
							
									// Wait for calculating the CRC to finish
									nvm_issue_flash_range_crc(BOOT_SECTION_START, BOOT_SECTION_START + i);
									nvm_wait_until_ready();
									while(CRC.STATUS & CRC_BUSY_bm);
				
									// Send bootloader CRC step
									strcpy(responseBuffer, i == 1 ? "0x" : " 0x");
									ltoa(CRC.CHECKSUM3, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM2, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									if(i != BOOT_SECTION_SIZE - 1)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get application CRC steps
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application CRC steps")) {
							
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
							
								// Go through every other byte in the application
								for(uint16_t i = 1; i < APP_SECTION_SIZE; i += 2) {
							
									// Wait until non-volatile memory controller isn't busy
									nvm_wait_until_ready();
				
									// Set CRC to use 0xFFFFFFFF seed and target flash memory
									CRC.CTRL = CRC_RESET_RESET1_gc;
									CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;
							
									// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise. This is an error in Atmel's ASF library)
									#if FLASH_SIZE < 0x10000UL
										NVM.ADDR2 = 0;
										NVM.DATA2 = 0;
									#endif
							
									// Wait for calculating the CRC to finish
									nvm_issue_flash_range_crc(APP_SECTION_START, APP_SECTION_START + i);
									nvm_wait_until_ready();
									while(CRC.STATUS & CRC_BUSY_bm);
				
									// Send application CRC step
									strcpy(responseBuffer, i == 1 ? "0x" : " 0x");
									ltoa(CRC.CHECKSUM3, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM2, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									if(i != BOOT_SECTION_SIZE - 1)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get application table CRC steps
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application table CRC steps")) {
							
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
							
								// Go through every other byte in the application table
								for(uint16_t i = 1; i < APPTABLE_SECTION_SIZE; i += 2) {
							
									// Wait until non-volatile memory controller isn't busy
									nvm_wait_until_ready();
				
									// Set CRC to use 0xFFFFFFFF seed and target flash memory
									CRC.CTRL = CRC_RESET_RESET1_gc;
									CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;
							
									// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise. This is an error in Atmel's ASF library)
									#if FLASH_SIZE < 0x10000UL
										NVM.ADDR2 = 0;
										NVM.DATA2 = 0;
									#endif
							
									// Wait for calculating the CRC to finish
									nvm_issue_flash_range_crc(APPTABLE_SECTION_START, APPTABLE_SECTION_START + i);
									nvm_wait_until_ready();
									while(CRC.STATUS & CRC_BUSY_bm);
				
									// Send application table CRC step
									strcpy(responseBuffer, i == 1 ? "0x" : " 0x");
									ltoa(CRC.CHECKSUM3, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM2, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									strcat(responseBuffer, " 0x");
									ltoa(CRC.CHECKSUM0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									if(i != BOOT_SECTION_SIZE - 1)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get application CRC
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application CRC")) {
							
								// Wait until non-volatile memory controller isn't busy
								nvm_wait_until_ready();
					
								// Set CRC to use 0xFFFFFFFF seed and target flash memory
								CRC.CTRL = CRC_RESET_RESET1_gc;
								CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;
								
								// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise. This is an error in Atmel's ASF library)
								#if FLASH_SIZE < 0x10000UL
									NVM.ADDR2 = 0;
									NVM.DATA2 = 0;
								#endif
								
								// Wait for calculating the CRC to finish
								nvm_issue_flash_range_crc(APP_SECTION_START, APP_SECTION_END);
								nvm_wait_until_ready();
								while(CRC.STATUS & CRC_BUSY_bm);
					
								// Send application CRC
								strcpy(responseBuffer, "ok\n0x");
								ltoa(CRC.CHECKSUM3, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM2, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
							
							// Otherwise check if host command is to get application CRC
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application table CRC")) {
							
								// Wait until non-volatile memory controller isn't busy
								nvm_wait_until_ready();
					
								// Set CRC to use 0xFFFFFFFF seed and target flash memory
								CRC.CTRL = CRC_RESET_RESET1_gc;
								CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;
								
								// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise. This is an error in Atmel's ASF library)
								#if FLASH_SIZE < 0x10000UL
									NVM.ADDR2 = 0;
									NVM.DATA2 = 0;
								#endif
								
								// Wait for calculating the CRC to finish
								nvm_issue_flash_range_crc(APPTABLE_SECTION_START, APPTABLE_SECTION_END);
								nvm_wait_until_ready();
								while(CRC.STATUS & CRC_BUSY_bm);
					
								// Send application table CRC
								strcpy(responseBuffer, "ok\n0x");
								ltoa(CRC.CHECKSUM3, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM2, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
					
							// Otherwise check if host command is to get bootloader CRC
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Bootloader CRC")) {
							
								// Wait until non-volatile memory controller isn't busy
								nvm_wait_until_ready();
					
								// Set CRC to use 0xFFFFFFFF seed and target flash memory
								CRC.CTRL = CRC_RESET_RESET1_gc;
								CRC.CTRL = CRC_CRC32_bm | CRC_SOURCE_FLASH_gc;
								
								// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise. This is an error in Atmel's ASF library)
								#if FLASH_SIZE < 0x10000UL
									NVM.ADDR2 = 0;
									NVM.DATA2 = 0;
								#endif
								
								// Wait for calculating the CRC to finish
								nvm_issue_flash_range_crc(BOOT_SECTION_START, BOOT_SECTION_END);
								nvm_wait_until_ready();
								while(CRC.STATUS & CRC_BUSY_bm);
					
								// Send bootloader CRC
								strcpy(responseBuffer, "ok\n0x");
								ltoa(CRC.CHECKSUM3, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM2, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(CRC.CHECKSUM0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
					
							// Otherwise check if host command is to get EEPROM
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "EEPROM")) {
					
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
					
								// Send EEPROM
								for(uint16_t i = EEPROM_START; i <= EEPROM_END; i++) {
									strcpy(responseBuffer, i == EEPROM_START ? "0x" : " 0x");
									ltoa(nvm_eeprom_read_byte(i), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									if(i != EEPROM_END)
										sendDataToUsb(responseBuffer);
								}
							}
					
							// Otherwise check if host command is to get user signature
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "User signature")) {
					
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
					
								// Send EEPROM
								for(uint16_t i = USER_SIGNATURES_START; i <= USER_SIGNATURES_END; i++) {
									strcpy(responseBuffer, i == USER_SIGNATURES_START ? "0x" : " 0x");
									ltoa(nvm_read_user_signature_row(i), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									if(i != USER_SIGNATURES_END)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get production signature
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Production signature")) {
					
								strcpy(responseBuffer, "ok\n");
								sendDataToUsb(responseBuffer);
					
								// Send EEPROM
								for(uint16_t i = PROD_SIGNATURES_START; i <= PROD_SIGNATURES_END; i++) {
									strcpy(responseBuffer, i == PROD_SIGNATURES_START ? "0x" : " 0x");
									ltoa(nvm_read_production_signature_row(i), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									if(i != PROD_SIGNATURES_END)
										sendDataToUsb(responseBuffer);
								}
							}
							
							// Otherwise check if host command is to get device ID
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Device ID")) {
					
								// Get device ID
								nvm_device_id deviceId;
								nvm_read_device_id(&deviceId);
								
								// Send device ID
								strcpy(responseBuffer, "ok\n0x");
								ltoa(deviceId.devid2, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceId.devid1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceId.devid0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
							
							// Otherwise check if host command is to get device revision
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Device revision")) {
					
								// Send device revision
								strcpy(responseBuffer, "ok\n0x");
								ltoa(nvm_read_device_rev(), numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
							
							// Otherwise check if host command is to get device serial
							else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Device serial")) {
					
								// Get device serial
								nvm_device_serial deviceSerial;
								nvm_read_device_serial(&deviceSerial);
								
								// Send device serial
								strcpy(responseBuffer, "ok\n0x");
								ltoa(deviceSerial.coordx1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.coordx0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.coordy1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.coordy0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.lotnum5, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.lotnum4, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.lotnum3, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.lotnum2, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.lotnum1, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.lotnum0, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
								strcat(responseBuffer, " 0x");
								ltoa(deviceSerial.wafnum, numberBuffer, 16);
								leadingPadBuffer(numberBuffer);
								strcat(responseBuffer, numberBuffer);
							}
					
							// Otherwise
							else
				
								// Set response to error
								strcpy(responseBuffer, "Error: Unknown host command");
						}

						// Otherwise
						else
					#endif
					{

						// Check if command has an N parameter
						if(requests[currentProcessingRequest].commandParameters & PARAMETER_N_OFFSET) {
		
							// Check if command doesn't have a valid checksum
							if(!(requests[currentProcessingRequest].commandParameters & VALID_CHECKSUM_OFFSET))

								// Set response to resend
								strcpy(responseBuffer, "rs");
			
							// Otherwise
							else {
							
								// Check if command is a starting command number
								if(requests[currentProcessingRequest].commandParameters & PARAMETER_M_OFFSET && requests[currentProcessingRequest].valueM == 110)

									// Set current command number
									currentCommandNumber = requests[currentProcessingRequest].valueN;
								
								// Otherwise check if current command number is at its max
								else if(currentCommandNumber == UINT64_MAX)

									// Set response to error
									strcpy(responseBuffer, "Error: Max command number exceeded");
	
								// Otherwise check if command has already been processed
								else if(requests[currentProcessingRequest].valueN < currentCommandNumber)
	
									// Set response to skip
									strcpy(responseBuffer, "skip");

								// Otherwise check if an older command was expected
								else if(requests[currentProcessingRequest].valueN > currentCommandNumber)

									// Set response to resend
									strcpy(responseBuffer, "rs");
							}
						}

						// Check if response wasn't set
						if(!*responseBuffer) {
		
							// Check if command has an M parameter
							if(requests[currentProcessingRequest].commandParameters & PARAMETER_M_OFFSET) {

								switch(requests[currentProcessingRequest].valueM) {
									
									// M17
									case 17:
					
										// Turn on motors
										motors.turnOn();
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
				
									// M18 or M84
									case 18:
									case 84:
					
										// Turn off motors
										motors.turnOff();
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// M82 or M83
									case 82:
									case 83:
					
										// Set extruder mode
										motors.extruderMode = requests[currentProcessingRequest].valueM == 82 ? ABSOLUTE : RELATIVE;
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
					
									// M104 or M109
									case 104:
									case 109:
									
										{
											// Check if temperature is valid
											int32_t temperature = requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET ? requests[currentProcessingRequest].valueS : HEATER_OFF_TEMPERATURE;
											if(temperature == HEATER_OFF_TEMPERATURE || (temperature >= HEATER_MIN_TEMPERATURE && temperature <= HEATER_MAX_TEMPERATURE)) {
						
												// Set response to if setting temperature was successful
												strcpy(responseBuffer, heater.setTemperature(temperature, requests[currentProcessingRequest].valueM == 109) ? "ok" : "Error: Heater calibration mode not supported");
												
												// Check if heater isn't working
												if(!heater.isWorking)
										
													// Set response to error
													strcpy(responseBuffer, "Error: Heater isn't working");
											}
						
											// Otherwise
											else
						
												// Set response to temperature range
												strcpy(responseBuffer, "Error: Temperature must be between " TOSTRING(HEATER_MIN_TEMPERATURE) " and " TOSTRING(HEATER_MAX_TEMPERATURE) " degrees Celsius");
										}
									break;
					
									// M105
									case 105:
		
										// Get temperature
										strcpy(responseBuffer, "ok\nT:");
										ftoa(heater.getTemperature(), numberBuffer);
										strcat(responseBuffer, numberBuffer);
										
										// Check if heater isn't working
										if(!heater.isWorking)
										
											// Set response to error
											strcpy(responseBuffer, "Error: Heater isn't working");
									break;
					
									// M106 or M107
									case 106:
									case 107:
					
										// Set fan's speed
										fan.setSpeed(requests[currentProcessingRequest].valueM == 106 && requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET ? requests[currentProcessingRequest].valueS : FAN_MIN_SPEED);
					
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
					
									// M114
									case 114:
									
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									
										// Go through all motors
										for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
										
											// Append motor's name to response
											switch(i) {
											
												case X:
													strcat(responseBuffer, "\nX:");
												break;
												
												case Y:
													strcat(responseBuffer, " Y:");
												break;
												
												case Z:
													strcat(responseBuffer, " Z:");
												break;
												
												case E:
												default:
													strcat(responseBuffer, " E:");
											}
											
											// Append motor's current value to response
											ftoa(motors.currentValues[i] * (motors.units == INCHES ? MILLIMETERS_TO_INCHES_SCALAR : 1), numberBuffer);
											strcat(responseBuffer, numberBuffer);
										}
									break;
			
									// M115
									case 115:
			
										// Check if command is to reset
										if(requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET && requests[currentProcessingRequest].valueS == 628) {
											
											// Disable interrupts
											cpu_irq_disable();
											
											// Go through X, Y, and Z axes	
											for(AXES currentSaveMotor = X; currentSaveMotor <= Z; currentSaveMotor = static_cast<AXES>(currentSaveMotor + 1))
											
												// Go through direction, validity, and value axes parameters
												for(AXES_PARAMETER currentSaveParameter = DIRECTION; currentSaveParameter <= VALUE; currentSaveParameter = static_cast<AXES_PARAMETER>(currentSaveParameter + 1))
											
													// Save current axis's parameter
													motors.saveState(currentSaveMotor, currentSaveParameter);
											
											// Wait until non-volatile memory controller isn't busy
											nvm_wait_until_ready();
											
											// Reset
											reset_do_soft_reset();
										}
			
										// Otherwise
										else {
			
											// Set response to device and firmware details
											strcpy(responseBuffer, "ok\nPROTOCOL:RepRap FIRMWARE_NAME:" TOSTRING(FIRMWARE_NAME) " FIRMWARE_VERSION:" TOSTRING(FIRMWARE_VERSION) " MACHINE_TYPE:Micro_3D EXTRUDER_COUNT:1 SERIAL_NUMBER:");
											strcat(responseBuffer, serialNumber);
										}
									break;
					
									// M117
									case 117:
									
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									
										// Go through X, Y, and Z motors
										for(uint8_t i = X; i <= Z; i++) {
										
											// Append motor's name to response
											switch(i) {
											
												case X:
													strcat(responseBuffer, "\nXV:");
												break;
												
												case Y:
													strcat(responseBuffer, " YV:");
												break;
												
												case Z:
												default:
													strcat(responseBuffer, " ZV:");
											}
											
											// Append motor's current validity to response
											strcat(responseBuffer, motors.currentStateOfValues[i] ? "1" : "0");
										}
									break;
									
									// Check if useless commands are allowed
									#if ALLOW_USELESS_COMMANDS == true
									
										// M404
										case 404:
									
											// Set response to reset cause
											strcpy(responseBuffer, "ok\nRC:");
											ulltoa(reset_cause_get_causes(), numberBuffer);
											strcat(responseBuffer, numberBuffer);
										break;
									#endif
					
									// M420
									case 420:
									
										// Set LED's brightness
										led.setBrightness(requests[currentProcessingRequest].commandParameters & PARAMETER_T_OFFSET ? requests[currentProcessingRequest].valueT : LED_MAX_BRIGHTNESS);
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// Check if useless commands are allowed
									#if ALLOW_USELESS_COMMANDS == true
									
										// M583
										case 583:
										
											// Set response to if gantry clips are detected
											strcpy(responseBuffer, "ok\nC");
											strcat(responseBuffer, motors.gantryClipsDetected() ? "1" : "0");
											
											// Check if accelerometer isn't working
											if(!motors.accelerometer.isWorking)
				
												// Set response to error
												strcpy(responseBuffer, "Error: Accelerometer isn't working");
										break;
									#endif
				
									// M618 or M619
									case 618:
									case 619:
									
										{
											// Check if EEPROM parameters are provided
											gcodeParameterOffset parameters = PARAMETER_S_OFFSET | PARAMETER_T_OFFSET | (requests[currentProcessingRequest].valueM == 618 ? PARAMETER_P_OFFSET : 0);
											if(requests[currentProcessingRequest].commandParameters & parameters) {
					
												// Check if parameters are valid
												if(requests[currentProcessingRequest].valueS >= EEPROM_FIRMWARE_VERSION_OFFSET && requests[currentProcessingRequest].valueT && requests[currentProcessingRequest].valueT <= sizeof(uint32_t) && requests[currentProcessingRequest].valueS + requests[currentProcessingRequest].valueT <= EEPROM_DECRYPTION_TABLE_OFFSET) {
							
													// Set response to offset
													strcpy(responseBuffer, "ok\nPT:");
													ulltoa(requests[currentProcessingRequest].valueS, numberBuffer);
													strcat(responseBuffer, numberBuffer);
								
													// Check if reading an EEPROM value
													if(requests[currentProcessingRequest].valueM == 619) {
								
														// Get value from EEPROM
														uint32_t value = 0;
														nvm_eeprom_read_buffer(requests[currentProcessingRequest].valueS, &value, requests[currentProcessingRequest].valueT);
							
														// Append value to response
														strcat(responseBuffer, " DT:");
														ulltoa(value, numberBuffer);
														strcat(responseBuffer, numberBuffer);
													}
								
													// Otherwise
													else {
						
														// Write value to EEPROM
														nvm_eeprom_erase_and_write_buffer(requests[currentProcessingRequest].valueS, &requests[currentProcessingRequest].valueP, requests[currentProcessingRequest].valueT);
													
														// Update bed changes
														motors.updateBedChanges();
													
														// Update heater changes
														heater.updateHeaterChanges();
													}
												}
											
												// Otherwise
												else
											
													// Set response to error
													strcpy(responseBuffer, "Error: Invalid address range");
											}
										}
									break;
									
									// Check if useless commands are allowed
									#if ALLOW_USELESS_COMMANDS == true
									
										// M5321
										case 5321:
									
											// Check if hours is provided
											if(requests[currentProcessingRequest].commandParameters & PARAMETER_X_OFFSET) {
										
												// Update hours counter in EEPROM
												float hoursCounter;
												nvm_eeprom_read_buffer(EEPROM_HOURS_COUNTER_OFFSET, &hoursCounter, EEPROM_HOURS_COUNTER_LENGTH);
												hoursCounter += requests[currentProcessingRequest].valueX;
												nvm_eeprom_erase_and_write_buffer(EEPROM_HOURS_COUNTER_OFFSET, &hoursCounter, EEPROM_HOURS_COUNTER_LENGTH);
											
												// Set response to confirmation
												strcpy(responseBuffer, "ok");
											}
										break;
									#endif
									
									// M20, M21, M22, M80, M81, M110, M111, M400, or M999
									case 20:
									case 21:
									case 22:
									case 80:
									case 81:
									case 110:
									case 111:
									case 400:
									case 999:
			
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
								}
							}
		
							// Otherwise check if command has a G parameter
							else if(requests[currentProcessingRequest].commandParameters & PARAMETER_G_OFFSET) {

								switch(requests[currentProcessingRequest].valueG) {
	
									// G0 or G1
									case 0:
									case 1:
								
										// Check if command doesn't contain an E value or the heater is on
										if(!(requests[currentProcessingRequest].commandParameters & PARAMETER_E_OFFSET) || heater.isHeating())
					
											// Set response to if movement was successful
											strcpy(responseBuffer, motors.move(requests[currentProcessingRequest]) ? "ok" : "Error: Movement is too big");
									
										// Otherwise
										else
									
											// Set response to error
											strcpy(responseBuffer, heater.isWorking ? "Error: Can't use the extruder when the heater is off" : "Error: Heater isn't working");
									break;
			
									// G4
									case 4:
									
										{
											// Delay specified number of milliseconds
											int32_t delayTime = requests[currentProcessingRequest].commandParameters & PARAMETER_P_OFFSET ? requests[currentProcessingRequest].valueP : 0;
											if(delayTime > 0)
												for(int32_t i = 0; i < delayTime; i++)
													delay_ms(1);
											
											// Delay specified number of seconds
											delayTime = requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET ? requests[currentProcessingRequest].valueS : 0;
											if(delayTime > 0)
												for(int32_t i = 0; i < delayTime; i++)
													delay_s(1);
										}
				
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
					
									// G28
									case 28:
					
										// Set response to if homing XY was successful
										strcpy(responseBuffer, motors.homeXY() ? "ok" : "Error: Accelerometer isn't working");
									break;
					
									// G30 or G32
									case 30:
									case 32:
									
										// Turn off fan and heater
										fan.setSpeed(FAN_MIN_SPEED);
										heater.setTemperature(HEATER_OFF_TEMPERATURE);
					
										// Set response to if calibrating was successful
										strcpy(responseBuffer, (requests[currentProcessingRequest].valueG == 30 ? motors.calibrateBedCenterZ0() : motors.calibrateBedOrientation()) ? "ok" : "Error: Accelerometer isn't working");
									break;
									
									// G33
									case 33:
					
										// Save Z as bed center Z0
										motors.saveZAsBedCenterZ0();
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
					
									// G90 or G91
									case 90:
									case 91:
					
										// Set modes
										motors.mode = motors.extruderMode = requests[currentProcessingRequest].valueG == 90 ? ABSOLUTE : RELATIVE;
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
					
									// G92
									case 92:
									
										// Go through all motors
										for(uint8_t i = 0; i < NUMBER_OF_MOTORS; i++) {
									
											// Get parameter offset and value
											gcodeParameterOffset parameterOffset;
											float *value;
											switch(i) {
										
												case X:
													parameterOffset = PARAMETER_X_OFFSET;
													value = &requests[currentProcessingRequest].valueX;
												break;
											
												case Y:
													parameterOffset = PARAMETER_Y_OFFSET;
													value = &requests[currentProcessingRequest].valueY;
												break;
											
												case Z:
													parameterOffset = PARAMETER_Z_OFFSET;
													value = &requests[currentProcessingRequest].valueZ;
												break;
											
												case E:
												default:
													parameterOffset = PARAMETER_E_OFFSET;
													value = &requests[currentProcessingRequest].valueE;
											}
											
											// Check if X, Y, Z, and E values aren't provided
											if(!(requests[currentProcessingRequest].commandParameters & (PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_Z_OFFSET | PARAMETER_E_OFFSET))) {
											
												// Set parameter offset to something that the command definitely has
												parameterOffset = PARAMETER_G_OFFSET;
											
												// Set value to zero
												*value = 0;
											}
										
											// Check if parameter is provided
											if(requests[currentProcessingRequest].commandParameters & parameterOffset) {
										
												// Disable saving motors state
												tc_set_overflow_interrupt_level(&MOTORS_SAVE_TIMER, TC_INT_LVL_OFF);
										
												// Set motors current value
												motors.currentValues[i] = *value * (motors.units == INCHES ? INCHES_TO_MILLIMETERS_SCALAR : 1);
											
												// Enable saving motors state
												tc_set_overflow_interrupt_level(&MOTORS_SAVE_TIMER, TC_INT_LVL_LO);
											}
										}
		
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// G20 or G21
									case 20:
									case 21:
			
										// Set units
										motors.units = requests[currentProcessingRequest].valueG == 20 ? INCHES : MILLIMETERS;
						
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
								}
							}
							
							// Otherwise check if command has parameter T
							else if(requests[currentProcessingRequest].commandParameters & PARAMETER_T_OFFSET)
			
								// Set response to confirmation
								strcpy(responseBuffer, "ok");
						}
	
						// Check if command has an N parameter and it was processed
						if(requests[currentProcessingRequest].commandParameters & PARAMETER_N_OFFSET && (!strncmp(responseBuffer, "ok", strlen("ok")) || !strncmp(responseBuffer, "rs", strlen("rs")) || !strncmp(responseBuffer, "skip", strlen("skip")))) {
						
							// Check if response is a confirmation
							if(!strncmp(responseBuffer, "ok", strlen("ok")))
							
								// Increment current command number
								currentCommandNumber++;
							
							// Append command number to response
							uint8_t endOfResponse = responseBuffer[0] == 's' ? strlen("skip") : strlen("ok");
							ulltoa(responseBuffer[0] == 'r' ? currentCommandNumber : requests[currentProcessingRequest].valueN, numberBuffer);
							memmove(&responseBuffer[endOfResponse + 1 + strlen(numberBuffer)], &responseBuffer[endOfResponse], strlen(responseBuffer) - 1);
							responseBuffer[endOfResponse] = ' ';
							memcpy(&responseBuffer[endOfResponse + 1], numberBuffer, strlen(numberBuffer));
						}
					}
					
					// Check if response wasn't set
					if(!*responseBuffer)
	
						// Set response to error
						strcpy(responseBuffer, "Error: Unknown G-code command");
				}
		
				// Append newline to response
				strcat(responseBuffer, "\n");
		
				// Send response if an emergency stop didn't happen
				if(!emergencyStopOccured)
					sendDataToUsb(responseBuffer);
			}
			
			// Clear request
			requests[currentProcessingRequest].isParsed = false;
			
			// Increment current processing request
			currentProcessingRequest = currentProcessingRequest == REQUEST_BUFFER_SIZE - 1 ? 0 : currentProcessingRequest + 1;
			
			// Enable sending wait responses
			enableSendingWaitResponses();
		}
		
		// Otherwise check if an emergency stop has occured
		else if(emergencyStopOccured) {
		
			// Disable sending wait responses
			disableSendingWaitResponses();
		
			// Reset all peripherals
			fan.setSpeed(FAN_MIN_SPEED);
			heater.reset();
			led.setBrightness(LED_MAX_BRIGHTNESS);
			motors.reset();
		
			// Clear emergency stop occured
			emergencyStopOccured = false;
			
			// Send confirmation
			sendDataToUsb("ok\n");
			
			// Enable sending wait responses
			enableSendingWaitResponses();
		}
	}
	
	// Return
	return EXIT_SUCCESS;
}


// Supporting function implementation
void cdcRxNotifyCallback(uint8_t port) {

	// Initialize variables
	static uint8_t currentReceivingRequest = 0;
	static uint8_t lastCharacterOffset = 0;
	static char accumulatedBuffer[UINT8_MAX + 1];
	
	// Get request
	uint8_t size = udi_cdc_get_nb_received_data();
	char buffer[UDI_CDC_COMM_EP_SIZE];
	udi_cdc_read_buf(buffer, size);
	
	// Prevent request from overflowing accumulated request
	if(size >= sizeof(accumulatedBuffer) - lastCharacterOffset)
		size = sizeof(accumulatedBuffer) - lastCharacterOffset - 1;
	buffer[size] = 0;
	
	// Accumulate requests
	strcpy(&accumulatedBuffer[lastCharacterOffset], buffer);
	lastCharacterOffset += size;
	
	// Check if no more data is available
	if(size != UDI_CDC_COMM_EP_SIZE) {
	
		// Clear last character offset
		lastCharacterOffset = 0;
	
		// Check if an emergency stop isn't being processed
		if(!emergencyStopOccured)
	
			// Go through all commands in request
			for(char *offset = accumulatedBuffer; *offset; offset++) {
	
				// Parse request
				Gcode gcode;
				gcode.parseCommand(offset);
	
				// Check if request is an unconditional, conditional, or emergency stop and it has a valid checksum if it has an N parameter
				if(gcode.commandParameters & PARAMETER_M_OFFSET && (gcode.valueM == 0 || gcode.valueM == 1 || gcode.valueM == 112) && (!(gcode.commandParameters & PARAMETER_N_OFFSET) || gcode.commandParameters & VALID_CHECKSUM_OFFSET))

					// Stop all peripherals
					heater.emergencyStopOccured = motors.emergencyStopOccured = emergencyStopOccured = true;

				// Otherwise check if currently receiving request is empty
				else if(!requests[currentReceivingRequest].isParsed) {
		
					// Set current receiving request to command
					memcpy(&requests[currentReceivingRequest], &gcode, sizeof(Gcode));
			
					// Increment current receiving request
					currentReceivingRequest = currentReceivingRequest == REQUEST_BUFFER_SIZE - 1 ? 0 : currentReceivingRequest + 1;
				}
			
				// Check if next command doesn't exist
				if(!(offset = strchr(offset, '\n')))
				
					// Break
					break;
			}
	}
}

void cdcDisconnectCallback(uint8_t port) {

	// Prepare to reattach to the host
	udc_detach();
	udc_attach();
}

void disableSendingWaitResponses() {

	// Disable sending wait responses
	tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_OFF);
}

void enableSendingWaitResponses() {

	// Reset wait timer counter
	waitTimerCounter = 0;
	
	// Enable sending wait responses
	tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_LO);
}
