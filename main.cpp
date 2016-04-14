// ATxmega32C4 http://www.atmel.com/Images/Atmel-8493-8-and-32-bit-AVR-XMEGA-Microcontrollers-ATxmega16C4-ATxmega32C4_Datasheet.pdf
// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include <ctype.h>
#include "common.h"
#include "eeprom.h"
#include "fan.h"
#include "gcode.h"
#include "heater.h"
#include "led.h"
#include "motors.h"


// Definitions
#define REQUEST_BUFFER_SIZE 10
#define WAIT_TIMER MOTORS_VREF_TIMER
#define WAIT_TIMER_PERIOD MOTORS_VREF_TIMER_PERIOD

// Firmware name and version
#ifndef FIRMWARE_NAME
	#define FIRMWARE_NAME iMe
#endif

#ifndef FIRMWARE_VERSION
	#define FIRMWARE_VERSION 1900000001
#endif

// Unknown pins
#define UNKNOWN_PIN_1 IOPORT_CREATE_PIN(PORTA, 1) // Connected to transistors above the microcontroller
#define UNKNOWN_PIN_2 IOPORT_CREATE_PIN(PORTA, 5) // Connected to a resistor and capacitor in parallel to ground


// Request structure
struct Request {
	
	// Size, buffer, and is emergency stop
	uint8_t size;
	char buffer[UDI_CDC_COMM_EP_SIZE + 1];
	bool isEmergencyStop;
};


// Global variables
char serialNumber[USB_DEVICE_GET_SERIAL_NAME_LENGTH];
Request requests[REQUEST_BUFFER_SIZE];
uint16_t waitCounter;
bool emergencyStopOccured = false;
Fan fan;
Heater heater;
Led led;
Motors motors;


// Function prototypes

/*
Name: CDC RX notify callback
Purpose: Callback for when USB receives data
*/
void cdcRxNotifyCallback(uint8_t port);


// Main function
int main() {

	// Initialize interrupt controller
	pmic_init();
	
	// Initialize interrupt vectors
	irq_initialize_vectors();
	
	// Initialize system clock
	sysclk_init();
	
	// Initialize board
	board_init();
	
	// Initialize I/O ports
	ioport_init();
	
	// Go through all requests
	for(uint8_t i = 0; i < REQUEST_BUFFER_SIZE; i++) {
	
		// Clear size
		requests[i].size = 0;
		
		// Clear is emergency stop
		requests[i].isEmergencyStop = false;
	}
	
	// Initialize variables
	uint64_t currentLineNumber = 0;
	uint8_t currentProcessingRequest = 0;
	char responseBuffer[255];
	char numberBuffer[sizeof("18446744073709551615")];
	Gcode gcode;
	
	// Initialize peripherals
	fan.initialize();
	heater.initialize();
	led.initialize();
	motors.initialize();
	
	// Configure unknown pins
	ioport_set_pin_dir(UNKNOWN_PIN_1, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(UNKNOWN_PIN_1, IOPORT_PIN_LEVEL_LOW);
	ioport_set_pin_dir(UNKNOWN_PIN_2, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(UNKNOWN_PIN_2, IOPORT_MODE_PULLDOWN);
	
	// Configure send wait interrupt
	tc_set_overflow_interrupt_callback(&WAIT_TIMER, []() -> void {
	
		// Check if time to send wait
		if(++waitCounter >= sysclk_get_cpu_hz() / WAIT_TIMER_PERIOD) {
		
			// Reset wait counter
			waitCounter = 0;
			
			// Send wait
			udi_cdc_write_buf("wait\n", strlen("wait\n"));
		}
	});
	
	// Read serial from EEPROM
	nvm_eeprom_read_buffer(EEPROM_SERIAL_NUMBER_OFFSET, serialNumber, EEPROM_SERIAL_NUMBER_LENGTH);
	
	// Enable interrupts
	cpu_irq_enable();
	
	// Initialize USB
	udc_start();
	
	// Enable send wait interrupt
	waitCounter = 0;
	tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_LO);
	
	// Main loop
	while(true) {
	
		// Delay to allow enough time for a response to be received
		delay_us(1);
		
		// Check if a current processing request is ready
		if(requests[currentProcessingRequest].size) {
		
			// Disable send wait interrupt
			tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_OFF);
			
			// Parse command
			gcode.parseCommand(requests[currentProcessingRequest].buffer);
			
			// Clear request buffer size
			requests[currentProcessingRequest].size = 0;
			
			// Increment current processing request
			uint8_t requestPosition = currentProcessingRequest;
			currentProcessingRequest = currentProcessingRequest == REQUEST_BUFFER_SIZE - 1 ? 0 : currentProcessingRequest + 1;
			
			// Check if an emergency stop has occured
			if(emergencyStopOccured) {
			
				// Check if request is an emergency stop
				if(requests[requestPosition].isEmergencyStop) {
				
					// Clear that request is an emergency stop
					requests[requestPosition].isEmergencyStop = false;
					
					// Clear peripherals emergency stop occured
					heater.emergencyStopOccured = motors.emergencyStopOccured = false;
				
					// Clear emergency stop occured
					emergencyStopOccured = false;
				}
				
				// Otherwise
				else
				
					// Skip command
					continue;
			}
			
			// Clear response buffer
			*responseBuffer = 0;
			
			// Check if accelerometer is working
			if(motors.accelerometer.isWorking) {
		
				// Check if command contains valid G-code
				if(gcode.commandParameters) {
			
					// Check if command has host command
					if(gcode.commandParameters & PARAMETER_HOST_COMMAND_OFFSET)
					
						// Set response to error
						strcpy(responseBuffer, "Error: Unknown host command");
				
					// Otherwise
					else {
			
						// Check if command has an N parameter
						if(gcode.commandParameters & PARAMETER_N_OFFSET) {
						
							// Check if command is a starting line number
							if(gcode.valueN == 0 && gcode.valueM == 110)
				
								// Reset current line number
								currentLineNumber = 0;
						
							// Check if command doesn't have a valid checksum
							if(!gcode.hasValidChecksum())
				
								// Set response to resend
								strcpy(responseBuffer, "rs");
							
							// Otherwise
							else {
					
								// Check if line number is correct
								if(gcode.valueN == currentLineNumber)
					
									// Increment current line number
									currentLineNumber++;
						
								// Otherwise check if command has already been processed
								else if(gcode.valueN < currentLineNumber)
						
									// Set response to skip
									strcpy(responseBuffer, "skip");
					
								// Otherwise
								else
					
									// Set response to resend
									strcpy(responseBuffer, "rs");
							}
						}
				
						// Check if response wasn't set
						if(!*responseBuffer) {
						
							// Check if command has an M parameter
							if(gcode.commandParameters & PARAMETER_M_OFFSET) {
				
								switch(gcode.valueM) {
								
									// M17
									case 17:
									
										// Turn on motors
										motors.turnOn();
										
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
								
									// M18
									case 18:
									
										// Turn off motors
										motors.turnOff();
										
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// M104 or M109
									case 104:
									case 109:
									
										// Check if temperature is valid
										int32_t temperature;
										temperature = gcode.commandParameters & PARAMETER_S_OFFSET ? gcode.valueS : 0;
										if(!temperature || (temperature >= MIN_TEMPERATURE && temperature <= MAX_TEMPERATURE)) {
										
											// Set temperature
											heater.setTemperature(temperature, temperature && gcode.valueM == 109);
									
											// Set response to confirmation
											strcpy(responseBuffer, "ok");
										}
										
										// Otherwise
										else
										
											// Set response to temperature range
											strcpy(responseBuffer, "Error: Temperature must be between " TOSTRING(MIN_TEMPERATURE) " and " TOSTRING(MAX_TEMPERATURE) " degrees Celsius");
									break;
									
									// M105
									case 105:
						
										// Set response to temperature
										strcpy(responseBuffer, "ok\nT:");
										ftoa(heater.getTemperature(), numberBuffer);
										strcat(responseBuffer, numberBuffer);
									break;
									
									// M106 or M107
									case 106:
									case 107:
									
										// Check if speed is valid
										int32_t speed;
										speed = gcode.valueM == 107 || !(gcode.commandParameters & PARAMETER_S_OFFSET) ? 0 : gcode.valueS;
										if(speed >= 0) {
								
											// Set fan's speed
											fan.setSpeed(min(255, speed));
										
											// Set response to confirmation
											strcpy(responseBuffer, "ok");
										}
									break;
									
									// M114
									case 114:
									
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
										
										// Append motors current X to response
										strcat(responseBuffer, " X:");
										ftoa(motors.currentValues[X], numberBuffer);
										strcat(responseBuffer, numberBuffer);
									
										// Append motors current Y to response
										strcat(responseBuffer, " Y:");
										ftoa(motors.currentValues[Y], numberBuffer);
										strcat(responseBuffer, numberBuffer);
										
										// Append motors current Z to response
										strcat(responseBuffer, " Z:");
										ftoa(motors.currentValues[Z], numberBuffer);
										strcat(responseBuffer, numberBuffer);
									
										// Append motors current E to response
										strcat(responseBuffer, " E:");
										ftoa(motors.currentValues[E], numberBuffer);
										strcat(responseBuffer, numberBuffer);
									break;
							
									// M115
									case 115:
							
										// Check if command is to reset
										if(gcode.valueS == 628)
							
											// Perform software reset
											reset_do_soft_reset();
							
										// Otherwise
										else {
							
											// Put device details into response
											strcpy(responseBuffer, "ok REPRAP_PROTOCOL:0 FIRMWARE_NAME:" TOSTRING(FIRMWARE_NAME) " FIRMWARE_VERSION:" TOSTRING(FIRMWARE_VERSION) " MACHINE_TYPE:The_Micro X-SERIAL_NUMBER:");
											strncat(responseBuffer, serialNumber, EEPROM_SERIAL_NUMBER_LENGTH);
										}
									break;
									
									// M117
									case 117:
									
										// Set response to valid values
										strcpy(responseBuffer, "ok XV:");
										strcat(responseBuffer, nvm_eeprom_read_byte(EEPROM_SAVED_X_STATE_OFFSET) ? "1" : "0");
										strcat(responseBuffer, " YV:");
										strcat(responseBuffer, nvm_eeprom_read_byte(EEPROM_SAVED_Y_STATE_OFFSET) ? "1" : "0");
										strcat(responseBuffer, " ZV:");
										strcat(responseBuffer, nvm_eeprom_read_byte(EEPROM_SAVED_Z_STATE_OFFSET) ? "1" : "0");
									break;
									
									// M420
									case 420:
									
										// Check if duty cycle is provided
										if(gcode.commandParameters & PARAMETER_T_OFFSET) {
										
											// Check if brightness is valid
											uint8_t brightness = gcode.valueT;
											
											if(brightness <= 100) {
											
												// Set LED's brightness
												led.setBrightness(brightness);
												
												// Set response to confirmation
												strcpy(responseBuffer, "ok");
											}
										}
									break;
								
									// M618 or M619
									case 618:
									case 619:
								
										// Check if EEPROM offset and length are provided
										if(gcode.commandParameters & (PARAMETER_S_OFFSET | PARAMETER_T_OFFSET)) {
									
											// Check if offset and length are valid
											int32_t offset = gcode.valueS;
											uint8_t length = gcode.valueT;
										
											if(offset >= 0 && length && length <= 4 && offset + length < EEPROM_SIZE) {
											
												// Set response to offset
												strcpy(responseBuffer, "ok PT:");
												ulltoa(offset, numberBuffer);
												strcat(responseBuffer, numberBuffer);
												
												// Check if reading an EEPROM value
												if(gcode.valueM == 619) {
												
													// Get value from EEPROM
													uint32_t value = 0;
													nvm_eeprom_read_buffer(offset, &value, length);
											
													// Append value to response
													strcat(responseBuffer, " DT:");
													ulltoa(value, numberBuffer);
													strcat(responseBuffer, numberBuffer);
												}
												
												// Otherwise check if EEPROM value is provided
												else if(gcode.commandParameters & PARAMETER_P_OFFSET) {
										
													// Get value
													int32_t value = gcode.valueP;
										
													// Write value to EEPROM
													nvm_eeprom_erase_and_write_buffer(offset, &value, length);
												}
												
												// Otherwise
												else
												
													// Clear response buffer
													*responseBuffer = 0;
											}
										}
									break;
									
									// M0, M21, M84, or M110
									case 0:
									case 21:
									case 84:
									case 110:
							
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
								}
							}
						
							// Otherwise check if command has a G parameter
							else if(gcode.commandParameters & PARAMETER_G_OFFSET) {
				
								switch(gcode.valueG) {
					
									// G0 or G1
									case 0:
									case 1:
									
										// Check if command contains an X, Y, Z, E, or F parameter
										if(gcode.commandParameters & (PARAMETER_X_OFFSET | PARAMETER_Y_OFFSET | PARAMETER_Z_OFFSET | PARAMETER_E_OFFSET | PARAMETER_F_OFFSET)) {
										
											// Move
											motors.move(gcode);
							
											// Set response to confirmation
											strcpy(responseBuffer, "ok");
										}
									break;
							
									// G4
									case 4:
							
										// Delay specified time
										uint32_t delayTime;
										if((delayTime = gcode.valueP + gcode.valueS * 1000))
											delay_ms(delayTime);
								
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// G28
									case 28:
									
										// Home XY
										motors.homeXY();
										
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// G30
									case 30:
									
										// Calibrate bed center Z0
										motors.calibrateBedCenterZ0();
										
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// G32
									case 32:
									
										// Calibrate bed orientation
										motors.calibrateBedOrientation();
										
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
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
									
										// Set mode to absolute
										motors.mode = gcode.valueG == 90 ? ABSOLUTE : RELATIVE;
										
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
									
									// G92
									case 92:
									
										// Set motors current E
										motors.currentValues[E] = gcode.commandParameters & PARAMETER_E_OFFSET ? gcode.valueE : 0;
							
										// Set response to confirmation
										strcpy(responseBuffer, "ok");
									break;
								}
							}
							
							// Otherwise check if command has parameter T
							else if(gcode.commandParameters & PARAMETER_T_OFFSET)
							
								// Set response to confirmation
								strcpy(responseBuffer, "ok");
						}
					
						// Check if command has an N parameter and it was processed
						if(gcode.commandParameters & PARAMETER_N_OFFSET && (!strncmp(responseBuffer, "ok", 2) || !strncmp(responseBuffer, "rs", 2) || !strncmp(responseBuffer, "skip", 4))) {
			
							// Append line number to response
							uint8_t endOfResponse = responseBuffer[0] == 's' ? 4 : 2;
							ulltoa(responseBuffer[0] == 'r' ? currentLineNumber : gcode.valueN, numberBuffer);
							memmove(&responseBuffer[endOfResponse + 1 + strlen(numberBuffer)], &responseBuffer[endOfResponse], strlen(responseBuffer) - 1);
							responseBuffer[endOfResponse] = ' ';
							memcpy(&responseBuffer[endOfResponse + 1], numberBuffer, strlen(numberBuffer));
						}
					}
				}
			}
			
			// Otherwise
			else
			
				// Set response to error
				strcpy(responseBuffer, "Error: Accelerometer isn't working");
			
			// Check if response wasn't set
			if(!*responseBuffer)
			
				// Set response to error
				strcpy(responseBuffer, "Error: Unknown G-code command\n");
			
			// Otherwise
			else
			
				// Append newline to response
				strcat(responseBuffer, "\n");
			
			// Send response
			udi_cdc_write_buf(responseBuffer, strlen(responseBuffer));
			
			// Enable send wait interrupt
			waitCounter = 0;
			tc_set_overflow_interrupt_level(&WAIT_TIMER, TC_INT_LVL_LO);
		}
	}
	
	// Return 0
	return 0;
}


// Supporting function implementation
void cdcRxNotifyCallback(uint8_t port) {

	// Initialize variables
	static uint8_t currentReceivingRequest = 0;

	// Check if currently receiving request is empty and an emergency stop isn't being processed
	if(!requests[currentReceivingRequest].size && !emergencyStopOccured) {
	
		// Get request
		requests[currentReceivingRequest].size = udi_cdc_multi_get_nb_received_data(port);
		udi_cdc_multi_read_buf(port, requests[currentReceivingRequest].buffer, requests[currentReceivingRequest].size);
		requests[currentReceivingRequest].buffer[requests[currentReceivingRequest].size] = 0;
		
		// Check if request isn't empty
		if(requests[currentReceivingRequest].size) {
		
			// Check if request contains an emergency stop
			char *emergencyStopOffset = strstr(requests[currentReceivingRequest].buffer, "M0");
			if(emergencyStopOffset && !isdigit(emergencyStopOffset[2])) {
			
				// Check if emergency stop isn't after the start of a host command, checksum, or comment
				char *characterOffset = strpbrk(requests[currentReceivingRequest].buffer, "@*;");
				if(!characterOffset || emergencyStopOffset < characterOffset) {
			
					// Stop all peripherals
					fan.setSpeed(0);
					heater.emergencyStop();
					led.setBrightness(100);
					motors.emergencyStop();
					
					// Set that command is emergency stop
					requests[currentReceivingRequest].isEmergencyStop = true;
					
					// Set that an emergency stop occured
					emergencyStopOccured = true;
				}
			}
			
			// Increment current receiving request
			currentReceivingRequest = currentReceivingRequest == REQUEST_BUFFER_SIZE - 1 ? 0 : currentReceivingRequest + 1;
		}
	}
}
