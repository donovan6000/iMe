// 5V 0.2A 25x25x7mm brushless axial DC fan with a PH2.0-2P connector


// Header files
extern "C" {
	#include <asf.h>
}
#include "common.h"
#include "eeprom.h"
#include "fan.h"
#include "led.h"


// Definitions
#define FAN_ENABLE_PIN IOPORT_CREATE_PIN(PORTE, 1)
#define FAN_CHANNEL TC_CCB
#define FAN_CHANNEL_CAPTURE_COMPARE TC_CCBEN


// Supporting function implementation
void Fan::initialize() {

	// Configure fan enable pin
	ioport_set_pin_dir(FAN_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	
	// Configure fan timer
	tc_enable(&FAN_TIMER);
	tc_set_wgm(&FAN_TIMER, TC_WG_SS);
	tc_write_period(&FAN_TIMER, FAN_TIMER_PERIOD);
	tc_enable_cc_channels(&FAN_TIMER, static_cast<tc_cc_channel_mask_enable_t>(FAN_CHANNEL_CAPTURE_COMPARE | LED_CHANNEL_CAPTURE_COMPARE));
	tc_write_clock_source(&FAN_TIMER, TC_CLKSEL_DIV64_gc);
	
	// Turn off fan
	setSpeed(FAN_MIN_SPEED);
}

void Fan::setSpeed(uint8_t speed) {

	// Get fan scale
	float fanScale;
	nvm_eeprom_read_buffer(EEPROM_FAN_SCALE_OFFSET, &fanScale, EEPROM_FAN_SCALE_LENGTH);
	
	// Set speed
	tc_write_cc(&FAN_TIMER, FAN_CHANNEL, speed <= FAN_MIN_SPEED ? 0 : (getValueInRange(speed, FAN_MIN_SPEED, FAN_MAX_SPEED) * fanScale + nvm_eeprom_read_byte(EEPROM_FAN_OFFSET_OFFSET)) * FAN_TIMER_PERIOD / FAN_MAX_SPEED);
}
