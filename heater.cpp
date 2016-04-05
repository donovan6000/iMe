// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include "heater.h"
#include "eeprom.h"


// Definitions

// Heater Pins
#define HEATER_MODE_SELECT_PIN IOPORT_CREATE_PIN(PORTE, 2)
#define HEATER_ENABLE_PIN IOPORT_CREATE_PIN(PORTA, 2)
#define HEATER_READ_POSITIVE_PIN IOPORT_CREATE_PIN(PORTA, 3)
#define HEATER_READ_NEGATIVE_PIN IOPORT_CREATE_PIN(PORTA, 4)

#define HEATER_READ_ADC ADCA
#define HEATER_READ_ADC_CHANNEL ADC_CH0
#define HEATER_READ_POSITIVE_INPUT ADCCH_POS_PIN3
#define HEATER_READ_NEGATIVE_INPUT ADCCH_NEG_PIN4


// Global variables
uint16_t temperature = 0;


// Function prototypes

/*
Name: Check temperature
Purpose: Checks and adjusts heater's temperature
*/
void checkTemperature();


// Supporting function implementation
Heater::Heater() {

	// Configure heater select, enable, and read
	ioport_set_pin_dir(HEATER_MODE_SELECT_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, IOPORT_PIN_LEVEL_LOW);
	
	ioport_set_pin_dir(HEATER_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(HEATER_ENABLE_PIN, IOPORT_PIN_LEVEL_HIGH);
	
	ioport_set_pin_dir(HEATER_READ_POSITIVE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_dir(HEATER_READ_NEGATIVE_PIN, IOPORT_DIR_INPUT);
	
	/*adc_config heaterReadAdc;
	adc_channel_config heaterReadAdcChannel;
	adc_read_configuration(&HEATER_READ_ADC, &heaterReadAdc);
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	
	adc_set_conversion_parameters(&heaterReadAdc, ADC_SIGN_ON, ADC_RES_12, ADC_REF_AREFA);
	adc_set_conversion_trigger(&heaterReadAdc, ADC_TRIG_MANUAL, 1, 0);
	adc_set_clock_rate(&heaterReadAdc, 200000);
	adcch_set_input(&heaterReadAdcChannel, HEATER_READ_POSITIVE_INPUT, HEATER_READ_NEGATIVE_INPUT, 1);
	
	adc_write_configuration(&HEATER_READ_ADC, &heaterReadAdc);
	adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	adc_enable(&HEATER_READ_ADC);
	
	// Delay enough time for ADC to initialize
	delay_ms(1);*/
	
	// Configure check temperature interrupt timer
	/*tc_enable(&TCC0);
	tc_set_wgm(&TCC0, TC_WG_NORMAL);
	tc_write_period(&TCC0, sysclk_get_cpu_hz() / 1024);
	tc_set_overflow_interrupt_level(&TCC0, TC_INT_LVL_LO);
	tc_set_overflow_interrupt_callback(&TCC0, checkTemperature);*/
}

void Heater::setTemperature(uint16_t value) {

	// Set mode to heat
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, IOPORT_PIN_LEVEL_HIGH);
	
	// Set temperature
	temperature = value;
	
	// Enable check temperature interrupt
	//tc_restart(&TCC0);
	//tc_write_clock_source(&TCC0, TC_CLKSEL_DIV1024_gc);
}

int16_t Heater::getTemperature() {

	// Turn on heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, IOPORT_PIN_LEVEL_HIGH);
	
	/*// Get value
	adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	int16_t value = adc_get_signed_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);*/
	int16_t value = 0;
	
	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, IOPORT_PIN_LEVEL_LOW);
	
	// Return value
	return value;
}

void Heater::turnOff() {

	// Disable check temperature interrupt
	//tc_write_clock_source(&TCC0, TC_CLKSEL_OFF_gc);
	
	// Clear temperature
	temperature = 0;

	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, IOPORT_PIN_LEVEL_LOW);
}

void checkTemperature() {

	// Send wait
	udi_cdc_write_buf("temp\n", strlen("temp\n"));
}
