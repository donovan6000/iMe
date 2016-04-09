// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include "heater.h"
#include "motors.h"
#include "eeprom.h"
#include "common.h"


// Definitions

// Heater Pins
#define HEATER_MODE_SELECT_PIN IOPORT_CREATE_PIN(PORTE, 2)
#define HEATER_ENABLE_PIN IOPORT_CREATE_PIN(PORTA, 2)
#define HEATER_READ_POSITIVE_PIN IOPORT_CREATE_PIN(PORTA, 3)
#define HEATER_READ_NEGATIVE_PIN IOPORT_CREATE_PIN(PORTA, 4)

// Heater ADC
#define HEATER_READ_ADC MOTOR_E_CURRENT_SENSE_ADC
#define HEATER_READ_ADC_CHANNEL ADC_CH0
#define HEATER_READ_POSITIVE_INPUT ADCCH_POS_PIN3
#define HEATER_READ_NEGATIVE_INPUT ADCCH_NEG_PIN4

// Pin states
#define HEATER_ON IOPORT_PIN_LEVEL_HIGH
#define HEATER_OFF IOPORT_PIN_LEVEL_LOW
#define HEATER_ENABLE IOPORT_PIN_LEVEL_HIGH
#define HEATER_DISABLE IOPORT_PIN_LEVEL_LOW


// Global variables
float idealTemperature = 0;
float actualTemperature = 0;
adc_channel_config heaterReadAdcChannel;


// Supporting function implementation
void Heater::initialize() {

	// Configure heater select, enable, and read
	ioport_set_pin_dir(HEATER_MODE_SELECT_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
	
	ioport_set_pin_dir(HEATER_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(HEATER_ENABLE_PIN, HEATER_ENABLE);
	
	ioport_set_pin_dir(HEATER_READ_POSITIVE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(HEATER_READ_POSITIVE_PIN, IOPORT_MODE_PULLDOWN);
	ioport_set_pin_dir(HEATER_READ_NEGATIVE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(HEATER_READ_NEGATIVE_PIN, IOPORT_MODE_PULLDOWN);
	
	// Read ADC channel configuration
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	
	// Set heater read pins to be a differential input
	adcch_set_input(&heaterReadAdcChannel, HEATER_READ_POSITIVE_INPUT, HEATER_READ_NEGATIVE_INPUT, 1);
	
	// Configure update temperature timer
	tc_enable(&TEMPERATURE_TIMER);
	tc_set_wgm(&TEMPERATURE_TIMER, TC_WG_NORMAL);
	tc_write_period(&TEMPERATURE_TIMER, sysclk_get_cpu_hz() / 1024);
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	tc_set_overflow_interrupt_callback(&TEMPERATURE_TIMER, []() -> void {
	
		// Turn on heater
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
	
		// Get heater temperature
		adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
		adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
		adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
		int16_t value = adc_get_signed_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	
		// Update actual temperature
		actualTemperature = 0;
		
		// Turn off heater
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
	});
}

void Heater::setTemperature(uint16_t value, bool wait) {
	
	// Check if heating
	if((idealTemperature = value)) {
	
		// Turn on heater
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
	
		// Start update temperature timer
		tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_DIV1024_gc);
	
		while(wait && ioport_get_pin_level(HEATER_MODE_SELECT_PIN) == HEATER_ON) {
		
			// Delay
			delay_s(1);
		
			// Pause update temperature timer
			tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_OFF_gc);
		
			// Set response to temperature
			char buffer[sizeof("18446744073709551615") + 6];
			ftoa(actualTemperature, buffer);
			strcat(buffer, "\n");
		
			// Send temperature
			udi_cdc_write_buf(buffer, strlen(buffer));
		
			// Resume update temperature timer
			tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_DIV1024_gc);
		}
	}
	
	// Otherwise
	else
	
		// Turn off heater
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
}

float Heater::getTemperature() const {

	// Return actual temperature
	return actualTemperature;
}

void Heater::emergencyStop() {

	// Stop update temperature timer
	tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_OFF_gc);

	// Clear ideal and actual temperature
	idealTemperature = actualTemperature = 0;

	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
}
