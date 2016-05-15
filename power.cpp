// Header files
extern "C" {
	#include <asf.h>
}
#include "power.h"
#include "common.h"


// Definitions
#define VOLTAGE_READ_ADC ADC_MODULE
#define VOLTAGE_READ_ADC_FREQUENCY 200000
#define VOLTAGE_READ_ADC_SAMPLE_SIZE 50
#define VOLTAGE_READ_ADC_CHANNEL ADC_CH0
#define VOLTAGE_READ_ADC_PIN ADCCH_POS_BANDGAP


// Supporting function implementation
void Power::initialize() {

	// Set ADC controller to use unsigned, 12-bit, Vcc refrence, and manual trigger
	adc_read_configuration(&VOLTAGE_READ_ADC, &voltageReadAdcController);
	adc_set_conversion_parameters(&voltageReadAdcController, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_VCC);
	adc_set_conversion_trigger(&voltageReadAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&voltageReadAdcController, VOLTAGE_READ_ADC_FREQUENCY);
	
	// Set ADC channel to use bandgap as single ended input with no gain
	adcch_read_configuration(&VOLTAGE_READ_ADC, VOLTAGE_READ_ADC_CHANNEL, &voltageReadAdcChannel);
	adcch_set_input(&voltageReadAdcChannel, VOLTAGE_READ_ADC_PIN, ADCCH_NEG_NONE, 1);
	
	// Set stable supply voltage
	stableSupplyVoltage = getSupplyVoltage();
}

float Power::getSupplyVoltage() {

	// Read supply voltages
	adc_write_configuration(&VOLTAGE_READ_ADC, &voltageReadAdcController);
	adcch_write_configuration(&VOLTAGE_READ_ADC, VOLTAGE_READ_ADC_CHANNEL, &voltageReadAdcChannel);
	
	uint32_t value = 0;
	for(uint8_t i = 0; i < VOLTAGE_READ_ADC_SAMPLE_SIZE; i++) {
		adc_start_conversion(&VOLTAGE_READ_ADC, VOLTAGE_READ_ADC_CHANNEL);
		adc_wait_for_interrupt_flag(&VOLTAGE_READ_ADC, VOLTAGE_READ_ADC_CHANNEL);
		value += adc_get_unsigned_result(&VOLTAGE_READ_ADC, VOLTAGE_READ_ADC_CHANNEL);
	}
	
	// Return average supply voltage
	value /= VOLTAGE_READ_ADC_SAMPLE_SIZE;
	return ADC_VCC_REFRENCE_DIVISION_FACTOR * BANDGAP_VOLTAGE * UINT12_MAX / value;
}
