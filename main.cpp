// Some of this firmware's code may look weird and go against proper programming conventions, but it was all intentionally done because it produced a smaller firmware
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
#define REQUEST_QUEUE_SIZE 15
#define WAIT_TIMER FAN_TIMER
#define WAIT_TIMER_PERIOD FAN_TIMER_PERIOD
#define EMERGENCY_STOP_NOW 1
#define NO_EMERGENCY_STOP 0
#define INACTIVITY_TIMEOUT_SECONDS (10 * 60)
#define REQUEST_BUFFER_SIZE (512 + 1)
#define RESPONSE_BUFFER_SIZE (sizeof("ok\nPROTOCOL:RepRap FIRMWARE_NAME:" TOSTRING(FIRMWARE_NAME) " FIRMWARE_VERSION:" TOSTRING(FIRMWARE_VERSION) " MACHINE_TYPE:Micro_3D EXTRUDER_COUNT:1 SERIAL_NUMBER:") - 1 + EEPROM_SERIAL_NUMBER_LENGTH + sizeof(' ') + INT_BUFFER_SIZE - 1 + 1)

// Unknown pin (Connected to transistors above the microcontroller. Maybe related to detecting if USB is connected)
#define UNKNOWN_PIN IOPORT_CREATE_PIN(PORTA, 1)

// Unused pins (None of them are connected to anything, so they could be used to easily interface additional hardware to the printer)
#define UNUSED_PIN_1 IOPORT_CREATE_PIN(PORTA, 6)
#define UNUSED_PIN_2 IOPORT_CREATE_PIN(PORTB, 0)
#define UNUSED_PIN_3 IOPORT_CREATE_PIN(PORTE, 0)
#define UNUSED_PIN_4 IOPORT_CREATE_PIN(PORTR, 0)
#define UNUSED_PIN_5 IOPORT_CREATE_PIN(PORTR, 1)

// Check if debug features are enabled
#if ENABLE_DEBUG_FEATURES == true

	// Heap start
	extern char __heap_start;

	// RAM mask
	#define RAM_MASK 0x41
#endif


// Global variables
char serialNumber[EEPROM_SERIAL_NUMBER_LENGTH + 1];
Gcode requests[REQUEST_QUEUE_SIZE];
uint8_t emergencyStopRequest;
uint8_t waitTimerCounter;
uint16_t inactivityCounter;
Fan fan;
Led led;
Heater heater;
Motors motors;


// Function prototypes

/*
Name: CDC RX notify callback
Purpose: Callback for when USB receives data
*/
void cdcRxNotifyCallback(uint8_t port) noexcept;

/*
Name: CDC disconnect callback
Purpose: Callback for when USB is disconnected from host (This should get called when the device is disconnected, but it gets called when the device is reattached which makes it necessary to guarantee that all data being sent over USB can actually be sent to prevent a buffer from overflowing when the device is disconnected. This is either a silicon error or an error in Atmel's ASF library)
*/
void cdcDisconnectCallback(uint8_t port) noexcept;

/*
Name: Disable sending wait responses
Purpose: Disables sending wait responses every second
*/
void disableSendingWaitResponses() noexcept;

/*
Name: Enable sending wait responses
Purpose: Enabled sending wait responses every second
*/
void enableSendingWaitResponses() noexcept;

/*
Name: Reverse bits
Purpose: Returns value with bits reversed
*/
uint32_t reverseBits(uint32_t value) noexcept;

/*
Name: Leading pad buffer
Purpose: Adds leading padding to buffer to make it meet the size specified
*/
void leadingPadBuffer(char *buffer, uint8_t size = 2, char padding = '0') noexcept;

/*
Name: Is alphanumeric
Purpose: Returns if a provided character is alphanumeric
*/
inline bool isAlphanumeric(char value) noexcept;

/*
Name: Update serial number
Purpose: Reads serial number from EEPROM
*/
void updateSerialNumber() noexcept;

/*
Name: Disable USB interrupts
Purpose: Disables USB interrupts
*/
void disableUsbInterrupts() noexcept;

/*
Name: Enable USB interrupts
Purpose: Enables USB interrupts
*/
void enableUsbInterrupts() noexcept;

/*
Name: Reset peripherals
Purpose: Resets all peripherals to their default state
*/
void resetPeripherals() noexcept;

// Check if debug features are enabled
#if ENABLE_DEBUG_FEATURES == true

	/*
	Name: Set RAM to mask
	Purpose: Sets all RAM to mask value
	*/
	void __attribute__((naked, used, section(".init3"))) setRamToMask() noexcept;
	
	/*
	Name: Get current unused RAM size
	Purpose: Returns the current amount of unused RAM
	*/
	inline ptrdiff_t getCurrentUnusedRamSize() noexcept;
	
	/*
	Name: Get total unused RAM size
	Purpose: Returns the total amount of unused RAM
	*/
	inline ptrdiff_t getTotalUnusedRamSize() noexcept;
#endif


// Main function
int __attribute__((OS_main)) main() noexcept {
	
	// Initialize system clock
	sysclk_init();
	
	// Initialize interrupt controller
	pmic_init();
	pmic_set_scheduling(PMIC_SCH_ROUND_ROBIN);
	
	// Initialize board
	board_init();
	
	// Initialize I/O ports
	ioport_init();
	
	// Initialize variables
	uint64_t currentCommandNumber = 0;
	static uint8_t currentProcessingRequest;
	static char responseBuffer[RESPONSE_BUFFER_SIZE];
	static char numberBuffer[INT_BUFFER_SIZE];
	
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
	tc_set_overflow_interrupt_callback(&WAIT_TIMER, []() noexcept -> void {
	
		// Check if one second has passed
		if(++waitTimerCounter >= sysclk_get_cpu_hz() / 64 / WAIT_TIMER_PERIOD) {
		
			// Reset wait timer counter
			waitTimerCounter = 0;
			
			// Send message
			#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
				char buffer[sizeof("wait\n")];
				strcpy_P(buffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("wait\n"))));
				sendDataToUsb(buffer, true);
			#else
				sendDataToUsb("wait\n", true);
			#endif
			
			// Check if a peripheral is on
			if(fan.isOn() || led.isOn() || heater.isOn() || motors.isOn()) {
			
				// Check if inactivity timeout seconds has passed
				if(++inactivityCounter >= INACTIVITY_TIMEOUT_SECONDS) {
			
					// Reset inactivity counter
					inactivityCounter = 0;
				
					// Reset peripherals
					resetPeripherals();
				
					// Send message
					#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
						char buffer[sizeof("Peripherals automatically turned off\n")];
						strcpy_P(buffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Peripherals automatically turned off\n"))));
						sendDataToUsb(buffer, true);
					#else
						sendDataToUsb("Peripherals automatically turned off\n", true);
					#endif
				}
			}
			
			// Otherwise
			else
			
				// Reset inactivity counter
				inactivityCounter = 0;
		}
	});
	
	// Fix writing to EEPROM addresses above 0x2E0 by first writing to an address less than that (This is either a silicon error or an error in Atmel's ASF library)
	nvm_eeprom_write_byte(0, nvm_eeprom_read_byte(0));
	
	// Update serial number
	updateSerialNumber();
	
	// Enable interrupts
	cpu_irq_enable();
	
	// Initialize USB
	udc_start();
	
	// Enable sending wait responses
	enableSendingWaitResponses();
	
	// Loop forever
	while(true)
	
		// Check if the current processing request is ready or an emergency stop occured right before current processing request
		if(requests[currentProcessingRequest].isParsed || emergencyStopRequest == EMERGENCY_STOP_NOW) {
		
			// Disable sending wait responses
			disableSendingWaitResponses();
	
			// Check if an emergency stop didn't occured right before current processing request
			if(emergencyStopRequest != EMERGENCY_STOP_NOW) {
	
				// Check if an emergency stop hasn't occured
				if(!emergencyStopRequest) {
	
					// Check if accelerometer isn't working
					if(!motors.accelerometer.isWorking && !motors.accelerometer.testConnection())
					
						// Set response to error
						#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
							strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Accelerometer isn't working"))));
						#else
							strcpy(responseBuffer, "Error: Accelerometer isn't working");
						#endif
	
					// Check if heater isn't working
					else if(!heater.isWorking && !heater.testConnection())
	
						// Set response to error
						#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
							strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Heater isn't working"))));
						#else
							strcpy(responseBuffer, "Error: Heater isn't working");
						#endif
	
					// Otherwise
					else {
	
						// Clear response buffer
						*responseBuffer = 0;

						// Check if host commands are allowed
						#if ALLOW_HOST_COMMANDS == true

							// Check if command is a host command
							if(requests[currentProcessingRequest].commandParameters & PARAMETER_HOST_COMMAND_OFFSET) {
			
								// Check if host command is to get lock bits
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Lock bits"))))) {
								#else
									if(!strcmp(requests[currentProcessingRequest].hostCommand, "Lock bits")) {
								#endif
		
									// Send lock bits
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(NVM.LOCK_BITS, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
								}
		
								// Otherwise check if host command is to get fuse bytes
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Fuse bytes"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Fuse bytes")) {
								#endif
		
									// Send fuse bytes
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(nvm_fuses_read(FUSEBYTE0), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(nvm_fuses_read(FUSEBYTE1), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(nvm_fuses_read(FUSEBYTE2), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(nvm_fuses_read(FUSEBYTE3), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(nvm_fuses_read(FUSEBYTE4), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(nvm_fuses_read(FUSEBYTE5), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
								}
		
								// Otherwise check if host command is to get application contents
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Application contents"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application contents")) {
								#endif
		
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
		
									// Send application
									for(uint16_t i = APP_SECTION_START; i <= APP_SECTION_END; ++i) {
									
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == APP_SECTION_START ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == APP_SECTION_START ? "0x" : " 0x");
										#endif
										
										ltoa(pgm_read_byte(i), numberBuffer, 16);
										leadingPadBuffer(numberBuffer);
										strcat(responseBuffer, numberBuffer);
										if(i != APP_SECTION_END)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get application table contents
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Application table contents"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application table contents")) {
								#endif
		
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
		
									// Send application
									for(uint16_t i = APPTABLE_SECTION_START; i <= APPTABLE_SECTION_END; ++i) {
									
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == APPTABLE_SECTION_START ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == APPTABLE_SECTION_START ? "0x" : " 0x");
										#endif
										
										ltoa(pgm_read_byte(i), numberBuffer, 16);
										leadingPadBuffer(numberBuffer);
										strcat(responseBuffer, numberBuffer);
										if(i != APPTABLE_SECTION_END)
											sendDataToUsb(responseBuffer);
									}
								}
		
								// Otherwise check if host command is to get bootloader contents
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Bootloader contents"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Bootloader contents")) {
								#endif
		
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
					
									// Send bootloader
									for(uint16_t i = BOOT_SECTION_START; i <= BOOT_SECTION_END; ++i) {
									
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == BOOT_SECTION_START ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == BOOT_SECTION_START ? "0x" : " 0x");
										#endif
										
										ltoa(pgm_read_byte(i), numberBuffer, 16);
										leadingPadBuffer(numberBuffer);
										strcat(responseBuffer, numberBuffer);
										if(i != BOOT_SECTION_END)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get bootloader CRC steps
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Bootloader CRC steps"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Bootloader CRC steps")) {
								#endif
				
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
				
									// Go through every other byte in the bootloader
									for(uint16_t i = 1; i < BOOT_SECTION_SIZE; i += 2) {
				
										// Set CRC to use 0xFFFFFFFF seed
										crc_set_initial_value(UINT32_MAX);
					
										// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise when targeting CRC_FLASH_RANGE. This is an error in Atmel's ASF library)
										#if FLASH_SIZE < 0x10000UL
											NVM.ADDR2 = 0;
											NVM.DATA2 = 0;
										#endif
					
										// Get bootloader table CRC
										uint32_t crc = reverseBits(~crc_flash_checksum(CRC_FLASH_RANGE, BOOT_SECTION_START, i + 1));
					
										// Send bootloader table CRC
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == 1 ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == 1 ? "0x" : " 0x");
										#endif
										
										ltoa(crc, numberBuffer, 16);
										leadingPadBuffer(numberBuffer, 8);
										strncat(responseBuffer, numberBuffer, 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strncat(responseBuffer, &numberBuffer[2], 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strncat(responseBuffer, &numberBuffer[4], 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strcat(responseBuffer, &numberBuffer[6]);
						
										if(i != BOOT_SECTION_SIZE - 1)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get application CRC steps
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Application CRC steps"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application CRC steps")) {
								#endif
				
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
				
									// Go through every other byte in the application
									for(uint16_t i = 1; i < APP_SECTION_SIZE; i += 2) {
				
										// Set CRC to use 0xFFFFFFFF seed
										crc_set_initial_value(UINT32_MAX);
					
										// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise when targeting CRC_FLASH_RANGE. This is an error in Atmel's ASF library)
										#if FLASH_SIZE < 0x10000UL
											NVM.ADDR2 = 0;
											NVM.DATA2 = 0;
										#endif
					
										// Get application CRC
										uint32_t crc = reverseBits(~crc_flash_checksum(CRC_FLASH_RANGE, APP_SECTION_START, i + 1));
					
										// Send application CRC
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == 1 ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == 1 ? "0x" : " 0x");
										#endif
										
										ltoa(crc, numberBuffer, 16);
										leadingPadBuffer(numberBuffer, 8);
										strncat(responseBuffer, numberBuffer, 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strncat(responseBuffer, &numberBuffer[2], 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strncat(responseBuffer, &numberBuffer[4], 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strcat(responseBuffer, &numberBuffer[6]);
						
										if(i != APP_SECTION_SIZE - 1)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get application table CRC steps
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Application table CRC steps"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application table CRC steps")) {
								#endif
				
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
				
									// Go through every other byte in the application table
									for(uint16_t i = 1; i < APPTABLE_SECTION_SIZE; i += 2) {
					
										// Set CRC to use 0xFFFFFFFF seed
										crc_set_initial_value(UINT32_MAX);
					
										// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise when targeting CRC_FLASH_RANGE. This is an error in Atmel's ASF library)
										#if FLASH_SIZE < 0x10000UL
											NVM.ADDR2 = 0;
											NVM.DATA2 = 0;
										#endif
					
										// Get application table CRC
										uint32_t crc = reverseBits(~crc_flash_checksum(CRC_FLASH_RANGE, APPTABLE_SECTION_START, i + 1));
					
										// Send application table CRC
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == 1 ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == 1 ? "0x" : " 0x");
										#endif
										
										ltoa(crc, numberBuffer, 16);
										leadingPadBuffer(numberBuffer, 8);
										strncat(responseBuffer, numberBuffer, 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strncat(responseBuffer, &numberBuffer[2], 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strncat(responseBuffer, &numberBuffer[4], 2);
										
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
										#else
											strcat(responseBuffer, " 0x");
										#endif
										
										strcat(responseBuffer, &numberBuffer[6]);
						
										if(i != APPTABLE_SECTION_SIZE - 1)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get application CRC
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Application CRC"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application CRC")) {
								#endif
				
									// Set CRC to use 0xFFFFFFFF seed
									crc_set_initial_value(UINT32_MAX);
					
									// Get application CRC
									uint32_t crc = reverseBits(~crc_flash_checksum(CRC_APP, 0, 0));
					
									// Send application CRC
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(crc, numberBuffer, 16);
									leadingPadBuffer(numberBuffer, 8);
									strncat(responseBuffer, numberBuffer, 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strncat(responseBuffer, &numberBuffer[2], 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strncat(responseBuffer, &numberBuffer[4], 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strcat(responseBuffer, &numberBuffer[6]);
								}
				
								// Otherwise check if host command is to get application CRC
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Application table CRC"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Application table CRC")) {
								#endif
				
									// Set CRC to use 0xFFFFFFFF seed
									crc_set_initial_value(UINT32_MAX);
					
									// Clear high address bytes if total flash size doesn't extend that far (They are left unchanged otherwise when targeting CRC_FLASH_RANGE. This is an error in Atmel's ASF library)
									#if FLASH_SIZE < 0x10000UL
										NVM.ADDR2 = 0;
										NVM.DATA2 = 0;
									#endif
					
									// Get application table CRC
									uint32_t crc = reverseBits(~crc_flash_checksum(CRC_FLASH_RANGE, APPTABLE_SECTION_START, APPTABLE_SECTION_SIZE));
					
									// Send application table CRC
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(crc, numberBuffer, 16);
									leadingPadBuffer(numberBuffer, 8);
									strncat(responseBuffer, numberBuffer, 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strncat(responseBuffer, &numberBuffer[2], 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strncat(responseBuffer, &numberBuffer[4], 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strcat(responseBuffer, &numberBuffer[6]);
								}
		
								// Otherwise check if host command is to get bootloader CRC
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Bootloader CRC"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Bootloader CRC")) {
								#endif
				
									// Set CRC to use 0xFFFFFFFF seed
									crc_set_initial_value(UINT32_MAX);
					
									// Get bootloader CRC
									uint32_t crc = reverseBits(~crc_flash_checksum(CRC_BOOT, 0, 0));
					
									// Send bootloader CRC
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(crc, numberBuffer, 16);
									leadingPadBuffer(numberBuffer, 8);
									strncat(responseBuffer, numberBuffer, 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strncat(responseBuffer, &numberBuffer[2], 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strncat(responseBuffer, &numberBuffer[4], 2);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									strcat(responseBuffer, &numberBuffer[6]);
								}
		
								// Otherwise check if host command is to get EEPROM
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("EEPROM"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "EEPROM")) {
								#endif
		
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
		
									// Send EEPROM
									for(uint16_t i = EEPROM_START; i <= EEPROM_END; ++i) {
									
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == EEPROM_START ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == EEPROM_START ? "0x" : " 0x");
										#endif
										
										ltoa(nvm_eeprom_read_byte(i), numberBuffer, 16);
										leadingPadBuffer(numberBuffer);
										strcat(responseBuffer, numberBuffer);
										if(i != EEPROM_END)
											sendDataToUsb(responseBuffer);
									}
								}
		
								// Otherwise check if host command is to get user signature
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("User signature"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "User signature")) {
								#endif
		
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
		
									// Send EEPROM
									for(uint16_t i = USER_SIGNATURES_START; i <= USER_SIGNATURES_END; ++i) {
									
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == USER_SIGNATURES_START ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == USER_SIGNATURES_START ? "0x" : " 0x");
										#endif
										
										ltoa(nvm_read_user_signature_row(i), numberBuffer, 16);
										leadingPadBuffer(numberBuffer);
										strcat(responseBuffer, numberBuffer);
										if(i != USER_SIGNATURES_END)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get production signature
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Production signature"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Production signature")) {
								#endif
		
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
									#else
										strcpy(responseBuffer, "ok\n");
									#endif
									
									sendDataToUsb(responseBuffer);
		
									// Send EEPROM
									for(uint16_t i = PROD_SIGNATURES_START; i <= PROD_SIGNATURES_END; ++i) {
									
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(i == PROD_SIGNATURES_START ? PSTR("0x") : PSTR(" 0x"))));
										#else
											strcpy(responseBuffer, i == PROD_SIGNATURES_START ? "0x" : " 0x");
										#endif
										
										ltoa(nvm_read_production_signature_row(i), numberBuffer, 16);
										leadingPadBuffer(numberBuffer);
										strcat(responseBuffer, numberBuffer);
										if(i != PROD_SIGNATURES_END)
											sendDataToUsb(responseBuffer);
									}
								}
				
								// Otherwise check if host command is to get device ID
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Device ID"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Device ID")) {
								#endif
		
									// Get device ID
									nvm_device_id deviceId;
									nvm_read_device_id(&deviceId);
					
									// Send device ID
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(deviceId.devid2, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceId.devid1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceId.devid0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
								}
				
								// Otherwise check if host command is to get device revision
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Device revision"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Device revision")) {
								#endif
		
									// Send device revision
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(nvm_read_device_rev(), numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
								}
				
								// Otherwise check if host command is to get device serial
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									else if(!strcmp_P(requests[currentProcessingRequest].hostCommand, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Device serial"))))) {
								#else
									else if(!strcmp(requests[currentProcessingRequest].hostCommand, "Device serial")) {
								#endif
		
									// Get device serial
									nvm_device_serial deviceSerial;
									nvm_read_device_serial(&deviceSerial);
					
									// Send device serial
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n0x"))));
									#else
										strcpy(responseBuffer, "ok\n0x");
									#endif
									
									ltoa(deviceSerial.coordx1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.coordx0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.coordy1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.coordy0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.lotnum5, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.lotnum4, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.lotnum3, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.lotnum2, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.lotnum1, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.lotnum0, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
									
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" 0x"))));
									#else
										strcat(responseBuffer, " 0x");
									#endif
									
									ltoa(deviceSerial.wafnum, numberBuffer, 16);
									leadingPadBuffer(numberBuffer);
									strcat(responseBuffer, numberBuffer);
								}
		
								// Otherwise
								else
	
									// Set response to error
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Unknown host command"))));
									#else
										strcpy(responseBuffer, "Error: Unknown host command");
									#endif
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
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("rs"))));
									#else
										strcpy(responseBuffer, "rs");
									#endif

								// Otherwise
								else {
				
									// Check if command is a starting command number
									if(requests[currentProcessingRequest].commandParameters & PARAMETER_M_OFFSET && requests[currentProcessingRequest].valueM == 110)

										// Set current command number
										currentCommandNumber = requests[currentProcessingRequest].valueN;
					
									// Otherwise check if current command number is at its max
									else if(currentCommandNumber == UINT64_MAX)

										// Set response to error
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Max command number exceeded"))));
										#else
											strcpy(responseBuffer, "Error: Max command number exceeded");
										#endif

									// Otherwise check if command has already been processed
									else if(requests[currentProcessingRequest].valueN < currentCommandNumber)

										// Set response to skip
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("skip"))));
										#else
											strcpy(responseBuffer, "skip");
										#endif

									// Otherwise check if an older command was expected
									else if(requests[currentProcessingRequest].valueN > currentCommandNumber)

										// Set response to resend
										#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
											strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("rs"))));
										#else
											strcpy(responseBuffer, "rs");
										#endif
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
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
	
										// M18 or M84
										case 18:
										case 84:
		
											// Turn off motors
											motors.turnOff();
			
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
						
										// M82 or M83
										case 82:
										case 83:
		
											// Set extruder mode
											motors.extruderMode = requests[currentProcessingRequest].valueM == 82 ? ABSOLUTE : RELATIVE;
			
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
		
										// M104 or M109
										case 104:
										case 109:
						
											{
												// Check if temperature is valid
												int32_t temperature = requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET ? requests[currentProcessingRequest].valueS : HEATER_OFF_TEMPERATURE;
												if(temperature == HEATER_OFF_TEMPERATURE || (temperature >= HEATER_MIN_TEMPERATURE && temperature <= HEATER_MAX_TEMPERATURE)) {
						
													// Set response to if setting temperature was successful
													#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
														strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(heater.setTemperature(temperature, requests[currentProcessingRequest].valueM == 109) ? PSTR("ok") : PSTR("Error: Heater calibration mode not supported"))));
													#else
														strcpy(responseBuffer, heater.setTemperature(temperature, requests[currentProcessingRequest].valueM == 109) ? "ok" : "Error: Heater calibration mode not supported");
													#endif
												
													// Check if heater isn't working
													if(!heater.isWorking)
										
														// Set response to error
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Heater isn't working"))));
														#else
															strcpy(responseBuffer, "Error: Heater isn't working");
														#endif
												}
												
												// Otherwise
												else
			
													// Set response to temperature range
													#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
														strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Temperature must be between " TOSTRING(HEATER_MIN_TEMPERATURE) " and " TOSTRING(HEATER_MAX_TEMPERATURE) " degrees Celsius"))));
													#else
														strcpy(responseBuffer, "Error: Temperature must be between " TOSTRING(HEATER_MIN_TEMPERATURE) " and " TOSTRING(HEATER_MAX_TEMPERATURE) " degrees Celsius");
													#endif
											}
										break;
		
										// M105
										case 105:

											// Get temperature
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\nT:"))));
											#else
												strcpy(responseBuffer, "ok\nT:");
											#endif
											
											ftoa(heater.getTemperature(), numberBuffer);
											strcat(responseBuffer, numberBuffer);
							
											// Check if heater isn't working
											if(!heater.isWorking)
							
												// Set response to error
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Heater isn't working"))));
												#else
													strcpy(responseBuffer, "Error: Heater isn't working");
												#endif
										break;
		
										// M106 or M107
										case 106:
										case 107:
		
											// Set fan's speed
											fan.setSpeed(requests[currentProcessingRequest].valueM == 106 && requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET ? requests[currentProcessingRequest].valueS : FAN_MIN_SPEED);
		
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
		
										// M114
										case 114:
						
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
											#else
												strcpy(responseBuffer, "ok\n");
											#endif
						
											// Go through all motors
											for(uint8_t i = 0; i < NUMBER_OF_MOTORS; ++i) {
							
												// Append motor's name to response
												switch(i) {
								
													case X:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("X:"))));
														#else
															strcat(responseBuffer, "X:");
														#endif
													break;
									
													case Y:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" Y:"))));
														#else
															strcat(responseBuffer, " Y:");
														#endif
													break;
									
													case Z:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" Z:"))));
														#else
															strcat(responseBuffer, " Z:");
														#endif
													break;
									
													case E:
													default:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" E:"))));
														#else
															strcat(responseBuffer, " E:");
														#endif
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
												for(Axes currentSaveMotor = X; currentSaveMotor <= Z; currentSaveMotor = static_cast<Axes>(currentSaveMotor + 1))
								
													// Go through direction, validity, and value axes parameters
													for(AxesParameter currentSaveParameter = DIRECTION; currentSaveParameter <= VALUE; currentSaveParameter = static_cast<AxesParameter>(currentSaveParameter + 1))
								
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
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\nPROTOCOL:RepRap FIRMWARE_NAME:" TOSTRING(FIRMWARE_NAME) " FIRMWARE_VERSION:" TOSTRING(FIRMWARE_VERSION) " MACHINE_TYPE:Micro_3D EXTRUDER_COUNT:1 SERIAL_NUMBER:"))));
												#else
													strcpy(responseBuffer, "ok\nPROTOCOL:RepRap FIRMWARE_NAME:" TOSTRING(FIRMWARE_NAME) " FIRMWARE_VERSION:" TOSTRING(FIRMWARE_VERSION) " MACHINE_TYPE:Micro_3D EXTRUDER_COUNT:1 SERIAL_NUMBER:");
												#endif
												
												strcat(responseBuffer, serialNumber);
											}
										break;
		
										// M117
										case 117:
						
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
											#else
												strcpy(responseBuffer, "ok\n");
											#endif
						
											// Go through X, Y, and Z motors
											for(uint8_t i = X; i <= Z; ++i) {
							
												// Append motor's name to response
												switch(i) {
								
													case X:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("XV:"))));
														#else
															strcat(responseBuffer, "XV:");
														#endif
													break;
									
													case Y:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" YV:"))));
														#else
															strcat(responseBuffer, " YV:");
														#endif
													break;
									
													case Z:
													default:
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" ZV:"))));
														#else
															strcat(responseBuffer, " ZV:");
														#endif
												}
								
												// Append motor's current validity to response
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(motors.currentStateOfValues[i] ? PSTR("1") : PSTR("0"))));
												#else
													strcat(responseBuffer, motors.currentStateOfValues[i] ? "1" : "0");
												#endif
											}
										break;
						
										// Check if useless commands are allowed
										#if ALLOW_USELESS_COMMANDS == true
						
											// M404
											case 404:
						
												// Set response to reset cause
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\nRC:"))));
												#else
													strcpy(responseBuffer, "ok\nRC:");
												#endif
												
												ulltoa(reset_cause_get_causes(), numberBuffer);
												strcat(responseBuffer, numberBuffer);
											break;
										#endif
		
										// M420
										case 420:
						
											// Set LED's brightness
											led.setBrightness(requests[currentProcessingRequest].commandParameters & PARAMETER_T_OFFSET ? requests[currentProcessingRequest].valueT : LED_MAX_BRIGHTNESS);
			
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
						
										// Check if useless commands are allowed
										#if ALLOW_USELESS_COMMANDS == true
						
											// M583
											case 583:
							
												// Set response to if gantry clips are detected
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\nC"))));
													strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(motors.gantryClipsDetected() ? PSTR("1") : PSTR("0"))));
												#else
													strcpy(responseBuffer, "ok\nC");
													strcat(responseBuffer, motors.gantryClipsDetected() ? "1" : "0");
												#endif
												
												// Check if accelerometer isn't working
												if(!motors.accelerometer.isWorking)
	
													// Set response to error
													#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
														strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Accelerometer isn't working"))));
													#else
														strcpy(responseBuffer, "Error: Accelerometer isn't working");
													#endif
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
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\nPT:"))));
														#else
															strcpy(responseBuffer, "ok\nPT:");
														#endif
														
														ulltoa(requests[currentProcessingRequest].valueS, numberBuffer);
														strcat(responseBuffer, numberBuffer);
					
														// Check if reading an EEPROM value
														if(requests[currentProcessingRequest].valueM == 619) {
					
															// Get value from EEPROM
															uint32_t value = 0;
															nvm_eeprom_read_buffer(requests[currentProcessingRequest].valueS, &value, requests[currentProcessingRequest].valueT);
				
															// Append value to response
															#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
																strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR(" DT:"))));
															#else
																strcat(responseBuffer, " DT:");
															#endif
															
															ulltoa(value, numberBuffer);
															strcat(responseBuffer, numberBuffer);
														}
					
														// Otherwise
														else {
			
															// Write value to EEPROM
															nvm_eeprom_erase_and_write_buffer(requests[currentProcessingRequest].valueS, &requests[currentProcessingRequest].valueP, requests[currentProcessingRequest].valueT);
														
															// Update serial number
															updateSerialNumber();
										
															// Update bed changes
															motors.updateBedChanges();
										
															// Update heater changes
															heater.updateHeaterChanges();
														}
													}
								
													// Otherwise
													else
								
														// Set response to error
														#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
															strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Invalid address range"))));
														#else
															strcpy(responseBuffer, "Error: Invalid address range");
														#endif
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
													#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
														strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
													#else
														strcpy(responseBuffer, "ok");
													#endif
												}
											break;
										#endif
										
										// Check if debug features are enabled
										#if ENABLE_DEBUG_FEATURES == true
										
											// M6000
											case 6000:
											
												// Set response to current unused RAM
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
												#else
													strcpy(responseBuffer, "ok\n");
												#endif
												
												ulltoa(getCurrentUnusedRamSize(), numberBuffer);
												strcat(responseBuffer, numberBuffer);
											break;
										
											// M6001
											case 6001:
											
												// Set response to total unused RAM
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
												#else
													strcpy(responseBuffer, "ok\n");
												#endif
												
												ulltoa(getTotalUnusedRamSize(), numberBuffer);
												strcat(responseBuffer, numberBuffer);
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
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
									}
								}

								// Otherwise check if command has a G parameter
								else if(requests[currentProcessingRequest].commandParameters & PARAMETER_G_OFFSET) {

									switch(requests[currentProcessingRequest].valueG) {

										// G0 or G1
										case 0:
										case 1:
					
											// Check if command doesn't contain an E value or the heater is on
											if(!(requests[currentProcessingRequest].commandParameters & PARAMETER_E_OFFSET) || heater.isOn())
		
												// Set response to if movement was successful
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(motors.move(requests[currentProcessingRequest]) ? PSTR("ok") : PSTR("Error: Movement is too big"))));
												#else
													strcpy(responseBuffer, motors.move(requests[currentProcessingRequest]) ? "ok" : "Error: Movement is too big");
												#endif
						
											// Otherwise
											else
						
												// Set response to error
												#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
													strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(heater.isWorking ? PSTR("Error: Can't use the extruder when the heater is off") : PSTR("Error: Heater isn't working"))));
												#else
													strcpy(responseBuffer, heater.isWorking ? "Error: Can't use the extruder when the heater is off" : "Error: Heater isn't working");
												#endif
										break;

										// G4
										case 4:
									
											// Delay until an emergency stop has occured or until total number of milliseconds have passed
											for(int32_t i = requests[currentProcessingRequest].commandParameters & PARAMETER_P_OFFSET ? requests[currentProcessingRequest].valueP : 0; i > 0 && !emergencyStopRequest; --i)
									
												// Delay one millisecond
												delayMilliseconds(1);
										
											// Delay until an emergency stop has occured or until total number of seconds have passed
											for(int32_t i = requests[currentProcessingRequest].commandParameters & PARAMETER_S_OFFSET ? requests[currentProcessingRequest].valueS : 0; i > 0 && !emergencyStopRequest; --i)
									
												// Delay one second
												delaySeconds(1);
	
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
		
										// G28
										case 28:
		
											// Set response to if homing XY was successful
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(motors.homeXY() ? PSTR("ok") : motors.accelerometer.isWorking ? PSTR("Error: Extruder is too high to home successfully") : PSTR("Error: Accelerometer isn't working"))));
											#else
												strcpy(responseBuffer, motors.homeXY() ? "ok" : motors.accelerometer.isWorking ? "Error: Extruder is too high to home successfully" : "Error: Accelerometer isn't working");
											#endif
										break;
		
										// G30 or G32
										case 30:
										case 32:
						
											// Turn off fan and heater
											fan.setSpeed(FAN_MIN_SPEED);
											heater.setTemperature(HEATER_OFF_TEMPERATURE);
		
											// Set response to if calibrating was successful
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr((requests[currentProcessingRequest].valueG == 30 ? motors.calibrateBedCenterZ0() : motors.calibrateBedOrientation()) ? PSTR("ok") : motors.accelerometer.isWorking ? PSTR("Error: Extruder is too high to home successfully") : PSTR("Error: Accelerometer isn't working"))));
											#else
												strcpy(responseBuffer, (requests[currentProcessingRequest].valueG == 30 ? motors.calibrateBedCenterZ0() : motors.calibrateBedOrientation()) ? "ok" : motors.accelerometer.isWorking ? "Error: Extruder is too high to home successfully" : "Error: Accelerometer isn't working");
											#endif
										break;
						
										// G33
										case 33:
		
											// Save Z as bed center Z0
											motors.saveZAsBedCenterZ0();
			
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
		
										// G90 or G91
										case 90:
										case 91:
		
											// Set modes
											motors.mode = motors.extruderMode = requests[currentProcessingRequest].valueG == 90 ? ABSOLUTE : RELATIVE;
			
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
		
										// G92
										case 92:
						
											// Disable saving motors state
											tc_set_overflow_interrupt_level(&MOTORS_SAVE_TIMER, TC_INT_LVL_OFF);
						
											// Go through all motors
											for(uint8_t i = 0; i < NUMBER_OF_MOTORS; ++i) {
						
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
												if(requests[currentProcessingRequest].commandParameters & parameterOffset)
									
													// Set motors current value
													motors.currentValues[i] = *value * (motors.units == INCHES ? INCHES_TO_MILLIMETERS_SCALAR : 1);
											}
							
											// Enable saving motors state
											tc_set_overflow_interrupt_level(&MOTORS_SAVE_TIMER, TC_INT_LVL_LO);

											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
										break;
						
										// G20 or G21
										case 20:
										case 21:

											// Set units
											motors.units = requests[currentProcessingRequest].valueG == 20 ? INCHES : MILLIMETERS;
			
											// Set response to confirmation
											#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
												strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
											#else
												strcpy(responseBuffer, "ok");
											#endif
									}
								}
				
								// Otherwise check if command has parameter T
								else if(requests[currentProcessingRequest].commandParameters & PARAMETER_T_OFFSET)

									// Set response to confirmation
									#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
										strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))));
									#else
										strcpy(responseBuffer, "ok");
									#endif
							}

							// Check if command has an N parameter and it was processed
							#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
								if(requests[currentProcessingRequest].commandParameters & PARAMETER_N_OFFSET && (!strncmp_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))), sizeof("ok") - 1) || !strncmp_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("rs"))), sizeof("rs") - 1) || !strncmp_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("skip"))), sizeof("skip") - 1))) {
							#else
								if(requests[currentProcessingRequest].commandParameters & PARAMETER_N_OFFSET && (!strncmp(responseBuffer, "ok", sizeof("ok") - 1) || !strncmp(responseBuffer, "rs", sizeof("rs") - 1) || !strncmp(responseBuffer, "skip", sizeof("skip") - 1))) {
							#endif
							
								// Check if response is a confirmation
								#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
									if(!strncmp_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok"))), sizeof("ok") - 1))
								#else
									if(!strncmp(responseBuffer, "ok", sizeof("ok") - 1))
								#endif
				
									// Increment current command number
									++currentCommandNumber;
				
								// Append command number to response
								uint8_t endOfResponse = responseBuffer[0] == 's' ? sizeof("skip") - 1 : sizeof("ok") - 1;
								ulltoa(responseBuffer[0] == 'r' ? currentCommandNumber : requests[currentProcessingRequest].valueN, numberBuffer);
								memmove(&responseBuffer[endOfResponse + 1 + strlen(numberBuffer)], &responseBuffer[endOfResponse], strlen(responseBuffer) - 1);
								responseBuffer[endOfResponse] = ' ';
								memcpy(&responseBuffer[endOfResponse + 1], numberBuffer, strlen(numberBuffer));
							}
						}
		
						// Check if response wasn't set
						if(!*responseBuffer)

							// Set response to error
							#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
								strcpy_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("Error: Unknown G-code command"))));
							#else
								strcpy(responseBuffer, "Error: Unknown G-code command");
							#endif
					}

					// Append newline to response
					#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
						strcat_P(responseBuffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("\n"))));
					#else
						strcat(responseBuffer, "\n");
					#endif

					// Send response if an emergency stop didn't happen
					if(!emergencyStopRequest)
						sendDataToUsb(responseBuffer);
				}
			
				// Disable USB interrupts
				disableUsbInterrupts();
		
				// Clear request
				requests[currentProcessingRequest].isParsed = false;
			
				// Check if an emergency stop didn't happen
				if(!emergencyStopRequest)
			
					// Enable USB interrupts
					enableUsbInterrupts();
			
				// Otherwise
				else
			
					// Decrement emergency stop request
					--emergencyStopRequest;
			
				// Increment current processing request
				currentProcessingRequest = currentProcessingRequest == REQUEST_QUEUE_SIZE - 1 ? 0 : currentProcessingRequest + 1;
			}
			
			// Otherwise
			else  {
		
				// Reset peripherals
				resetPeripherals();
		
				// Clear emergency stop request
				emergencyStopRequest = NO_EMERGENCY_STOP;
			
				// Enable USB interrupts
				enableUsbInterrupts();
			
				// Send confirmation
				#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
					char buffer[sizeof("ok\n")];
					strcpy_P(buffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("ok\n"))));
					sendDataToUsb(buffer);
				#else
					sendDataToUsb("ok\n");
				#endif
			}
			
			// Enable sending wait responses
			enableSendingWaitResponses();
		}
	
	// Return success
	return EXIT_SUCCESS;
}


// Supporting function implementation
void cdcRxNotifyCallback(uint8_t port) noexcept {

	// Initialize variables
	static uint8_t currentReceivingRequest;
	static uint16_t lastCharacterOffset;
	static char accumulatedBuffer[REQUEST_BUFFER_SIZE];
	
	// Get request
	uint8_t size = udi_cdc_multi_get_nb_received_data(UDI_CDC_PORT_NB - 1);
	bool complete = size != UDI_CDC_COMM_EP_SIZE;
	
	// Prevent request from overflowing accumulated request
	if(size >= sizeof(accumulatedBuffer) - lastCharacterOffset)
		size = sizeof(accumulatedBuffer) - lastCharacterOffset - 1;
	
	// Accumulate requests
	udi_cdc_multi_read_buf_and_ignore(UDI_CDC_PORT_NB - 1, &accumulatedBuffer[lastCharacterOffset], size);
	lastCharacterOffset += size;
	
	// Check if all of the request has been received
	if(complete) {
	
		// Set terminating character in request
		accumulatedBuffer[lastCharacterOffset] = 0;
	
		// Clear last character offset
		lastCharacterOffset = 0;
	
		// Go through all commands in request
		for(char *offset = accumulatedBuffer; *offset; offset++) {

			// Parse request
			Gcode gcode;
			gcode.parseCommand(offset);

			// Check if request is an unconditional, conditional, or emergency stop and it has a valid checksum if it has an N parameter
			if(gcode.commandParameters & PARAMETER_M_OFFSET && (gcode.valueM == 0 || gcode.valueM == 1 || gcode.valueM == 112) && (!(gcode.commandParameters & PARAMETER_N_OFFSET) || gcode.commandParameters & VALID_CHECKSUM_OFFSET)) {
				
				// Disable USB interrupts
				disableUsbInterrupts();
				
				// Set emergency stop request
				emergencyStopRequest = EMERGENCY_STOP_NOW;
				
				// Go through all requests
				for(uint8_t i = 0; i < REQUEST_QUEUE_SIZE; ++i)
				
					// Check if request exists
					if(requests[i].isParsed)
					
						// Increment emergency stop request
						++emergencyStopRequest;
			}

			// Otherwise check if currently receiving request is empty
			else if(!requests[currentReceivingRequest].isParsed) {
	
				// Set current receiving request to command
				memcpy(&requests[currentReceivingRequest], &gcode, sizeof(Gcode));
		
				// Increment current receiving request
				currentReceivingRequest = currentReceivingRequest == REQUEST_QUEUE_SIZE - 1 ? 0 : currentReceivingRequest + 1;
			}
		
			// Check if next command doesn't exist
			if(!(offset = strchr(offset, '\n')))
			
				// Break
				break;
		}
	}
}

void cdcDisconnectCallback(uint8_t port) noexcept {

	// Prepare to reattach to the host
	udc_detach();
	udc_attach();
}

void disableSendingWaitResponses() noexcept {

	// Disable sending wait responses
	tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_OFF);
}

void enableSendingWaitResponses() noexcept {

	// Reset wait timer and inactivity counters
	waitTimerCounter = inactivityCounter = 0;
	
	// Enable sending wait responses
	tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_LO);
}

uint32_t reverseBits(uint32_t value) noexcept {

	// Initialize result
	uint32_t result = 0;
	
	// Go through all bits
	for(uint8_t i = 0; i < sizeof(value) * 8; ++i) {
	
		// Append bit to result
		result = (result << 1) + (value & 1);
		
		// Remove bit from value
		value >>= 1;
	}
	
	// Return result
	return result;
}

void leadingPadBuffer(char *buffer, uint8_t size, char padding) noexcept {

	// Check if buffer is smaller that the specified size
	uint8_t bufferSize = strlen(buffer);
	if(bufferSize < size) {
	
		// Shift buffer toward the end
		memmove(&buffer[size - bufferSize], buffer, bufferSize + 1);
		
		// Prepend padding to buffer
		memset(buffer, padding, size - bufferSize);
	}
}

bool isAlphanumeric(char value) noexcept {

	// Return if character is alphanumeric
	return (value >= '0' && value <= '9') || (lowerCase(value) >= 'a' && lowerCase(value) <= 'z');
}

void updateSerialNumber() noexcept {

	// Wait until non-volatile memory controller isn't busy
	nvm_wait_until_ready();

	// Enable EEPROM mapping
	eeprom_enable_mapping();

	// Go through all characters in serial number
	for(uint8_t i = 0; i < EEPROM_SERIAL_NUMBER_LENGTH; ++i) {
	
		// Read character as an underscore if it isn't alphanumeric
		char character = *reinterpret_cast<char *>(MAPPED_EEPROM_START + EEPROM_SERIAL_NUMBER_OFFSET + i);
		serialNumber[i] = isAlphanumeric(character) ? character : '_';
	}
	
	// Disable EEPROM mapping
	eeprom_disable_mapping();
}

void disableUsbInterrupts() noexcept {

	// Disable USB interrupts
	USB.INTCTRLA &= ~(USB_INTLVL1_bm | USB_INTLVL0_bm);
}

void enableUsbInterrupts() noexcept {

	// Enable USB interrupts
	USB.INTCTRLA |= UDD_USB_INT_LEVEL & (USB_INTLVL1_bm | USB_INTLVL0_bm);
}

void resetPeripherals() noexcept {

	// Reset all peripherals
	fan.reset();
	heater.reset();
	led.reset();
	motors.reset();
}

// Check if debug features are enabled
#if ENABLE_DEBUG_FEATURES == true

	void setRamToMask() noexcept {

		// Set RAM to mask
		__asm volatile (
			"ldi r30, lo8 (__heap_start)\n"
			"ldi r31, hi8 (__heap_start)\n"
			"ldi r24, %0\n"
			"ldi r25, hi8 (%1)\n"
			"0:\n"
			"st  Z+,  r24\n"
			"cpi r30, lo8 (%1)\n"
			"cpc r31, r25\n"
			"brne 0b\n"
			:
			: "i" (RAM_MASK), "i" (RAMEND)
		);
	}

	ptrdiff_t getCurrentUnusedRamSize() noexcept {
	
		// Return current unused ram size
		return reinterpret_cast<char *>(AVR_STACK_POINTER_REG) > &__heap_start ? reinterpret_cast<char *>(AVR_STACK_POINTER_REG) - &__heap_start : 0;
	}

	ptrdiff_t getTotalUnusedRamSize() noexcept {

		// Get location of last unused byte of RAM
		char *i;
		for(i = &__heap_start; i <= reinterpret_cast<char *>(RAMEND) && *i == RAM_MASK; ++i);

		// Return total amount of unused RAM
		return i > &__heap_start ? i - 1 - &__heap_start : 0;
	}
#endif
