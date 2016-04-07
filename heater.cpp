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
#define TEMPERATURE_TIMER TCC1

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
float temperature = 0;


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
	
	// Configure check temperature interrupt timer
	tc_enable(&TEMPERATURE_TIMER);
	tc_set_wgm(&TEMPERATURE_TIMER, TC_WG_NORMAL);
	tc_write_period(&TEMPERATURE_TIMER, sysclk_get_cpu_hz() / 1024);
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	tc_set_overflow_interrupt_callback(&TEMPERATURE_TIMER, []() -> void {
	
		// Set response to temperature
		char buffer[sizeof("18446744073709551615") + 6];
		ftoa(temperature, buffer);
		strcat(buffer, "\n");
		
		// Send temperature
		udi_cdc_write_buf(buffer, strlen(buffer));
	});
}

void Heater::setTemperature(uint16_t value, bool wait) {

	// Set mode to heat
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
	
	// Set temperature
	temperature = value;
	
	// Enable check temperature interrupt
	tc_restart(&TEMPERATURE_TIMER);
	tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_DIV1024_gc);
}

float Heater::getTemperature() {

	// Turn on heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
	
	// Get heater temperature
	adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	int16_t value = adc_get_signed_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	
	float temp = 0;
	
	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
	
	// Return value
	return temp;
}

void Heater::emergencyStop() {

	// Clear temperature
	temperature = 0;

	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
}
