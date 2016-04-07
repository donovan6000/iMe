// Header files
extern "C" {
	#include <asf.h>
}
#include "fan.h"
#include "eeprom.h"


// Definitions
#define FAN_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 1)
#define FAN_CHANNEL TC_CCB


// Supporting function implementation
void Fan::initialize() {

	// Configure fan enable pin
	ioport_set_pin_dir(FAN_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure fan timer
	tc_enable(&FAN_TIMER);
	tc_set_wgm(&FAN_TIMER, TC_WG_SS);
	tc_write_period(&FAN_TIMER, FAN_TIMER_PERIOD);
	tc_enable_cc_channels(&FAN_TIMER, static_cast<tc_cc_channel_mask_enable_t>(TC_CCBEN | TC_CCDEN));
	tc_write_clock_source(&FAN_TIMER, TC_CLKSEL_DIV64_gc);
	
	// Turn off fan
	setSpeed(0);
}

void Fan::setSpeed(uint8_t speed) {

	// Get fan offset and scale
	uint8_t fanOffset;
	float fanScale;
	nvm_eeprom_read_buffer(EEPROM_FAN_OFFSET_OFFSET, &fanOffset, EEPROM_FAN_OFFSET_LENGTH);
	nvm_eeprom_read_buffer(EEPROM_FAN_SCALE_OFFSET, &fanScale, EEPROM_FAN_SCALE_LENGTH);
	
	// Set speed
	tc_write_cc(&FAN_TIMER, FAN_CHANNEL, !speed ? 0 : (speed * fanScale + fanOffset) * FAN_TIMER_PERIOD / 255);
}
