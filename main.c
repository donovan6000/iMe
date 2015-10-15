// PORTE1 is Fan, active high
// PORTE3 is LED, active high

// Header files
#include <asf.h>
#include <avr/eeprom.h>
#define F_CPU 24000000UL
#include <util/delay.h>


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
	eeprom_read_block((void*)&serialNumber, (const void*)0x2EF, 16);
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
