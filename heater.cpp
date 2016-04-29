// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include <math.h>
#include "heater.h"
#include "motors.h"
#include "eeprom.h"
#include "common.h"


// Definitions
#define UPDATE_TEMPERATURE_PER_SECOND 2

// Heater Pins
#define HEATER_MODE_SELECT_PIN IOPORT_CREATE_PIN(PORTE, 2)
#define HEATER_ENABLE_PIN IOPORT_CREATE_PIN(PORTA, 2)
#define HEATER_READ_POSITIVE_PIN IOPORT_CREATE_PIN(PORTA, 3)
#define HEATER_READ_NEGATIVE_PIN IOPORT_CREATE_PIN(PORTA, 4)
#define RESISTANCE_READ_PIN IOPORT_CREATE_PIN(PORTA, 5)

// Heater ADC
#define HEATER_READ_ADC MOTOR_E_CURRENT_SENSE_ADC
#define HEATER_READ_ADC_CHANNEL ADC_CH0
#define HEATER_READ_POSITIVE_INPUT ADCCH_POS_PIN3
#define HEATER_READ_NEGATIVE_INPUT ADCCH_NEG_PIN4
#define RESISTANCE_READ_INPUT ADCCH_POS_PIN5

// Pin states
#define HEATER_ON IOPORT_PIN_LEVEL_HIGH
#define HEATER_OFF IOPORT_PIN_LEVEL_LOW
#define HEATER_ENABLE IOPORT_PIN_LEVEL_HIGH
#define HEATER_DISABLE IOPORT_PIN_LEVEL_LOW


// Global variables
bool heaterWorking = true;
uint8_t temperatureIntervalCounter;
float idealTemperature;
float actualTemperature;
adc_config heaterReadAdcController;
adc_channel_config heaterReadAdcChannel;
adc_config resistanceReadAdcController;
adc_channel_config resistanceReadAdcChannel;


// Supporting function implementation
void Heater::initialize() {

	// Configure heater select and enable pins
	ioport_set_pin_dir(HEATER_MODE_SELECT_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_dir(HEATER_ENABLE_PIN, IOPORT_DIR_OUTPUT);
	ioport_set_pin_level(HEATER_ENABLE_PIN, HEATER_ENABLE);
	
	// Configure heater read pins
	ioport_set_pin_dir(HEATER_READ_POSITIVE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(HEATER_READ_POSITIVE_PIN, IOPORT_MODE_PULLDOWN);
	ioport_set_pin_dir(HEATER_READ_NEGATIVE_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(HEATER_READ_NEGATIVE_PIN, IOPORT_MODE_PULLDOWN);
	
	// Configure resistance read pin
	ioport_set_pin_dir(RESISTANCE_READ_PIN, IOPORT_DIR_INPUT);
	ioport_set_pin_mode(RESISTANCE_READ_PIN, IOPORT_MODE_PULLDOWN);
	
	// Reset
	reset();
	
	// Set ADC heater controller to use signed, 12bit, bandgap refrence, manual trigger, 200kHz frequency
	adc_read_configuration(&HEATER_READ_ADC, &heaterReadAdcController);
	adc_set_conversion_parameters(&heaterReadAdcController, ADC_SIGN_ON, ADC_RES_12, ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&heaterReadAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&heaterReadAdcController, 200000);
	
	// Set ADC heater channel to use heater read pins as a differential input with 1/2x gain
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	adcch_set_input(&heaterReadAdcChannel, HEATER_READ_POSITIVE_INPUT, HEATER_READ_NEGATIVE_INPUT, 0);
	
	// Set ADC resistance controller to use unsigned, 12bit, bandgap refrence, manual trigger, 200kHz frequency
	adc_read_configuration(&HEATER_READ_ADC, &resistanceReadAdcController);
	adc_set_conversion_parameters(&resistanceReadAdcController, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&resistanceReadAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&resistanceReadAdcController, 200000);
	
	// Set ADC resistance channel to use resistance read pin as a single input with no gain
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &resistanceReadAdcChannel);
	adcch_set_input(&resistanceReadAdcChannel, RESISTANCE_READ_INPUT, ADCCH_NEG_NONE, 1);
	
	// Configure update temperature timer
	tc_enable(&TEMPERATURE_TIMER);
	tc_set_wgm(&TEMPERATURE_TIMER, TC_WG_NORMAL);
	tc_write_period(&TEMPERATURE_TIMER, sysclk_get_cpu_hz() / 1024 / UPDATE_TEMPERATURE_PER_SECOND);
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	tc_set_overflow_interrupt_callback(&TEMPERATURE_TIMER, []() -> void {
	
		// Increment temperature interval counter
		temperatureIntervalCounter++;
	
		// Check if setting the temperature
		if(idealTemperature) {
	
			// Turn on heater
			ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
	
			// Get heater value
			adc_write_configuration(&HEATER_READ_ADC, &heaterReadAdcController);
			adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
			
			int32_t heaterValue = 0;
			for(uint8_t i = 0; i < 50; i++) {
				adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
				adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
				heaterValue += adc_get_signed_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
			}
			heaterValue /= 50;
			
			// Check if heater isn't working
			if(heaterValue >= (pow(2, 12) / 2 - 1) / 2) {
			
				// Clear heater working
				heaterWorking = false;
				
				// Clear ideal and actual temperature
				idealTemperature = actualTemperature = 0;
			}
			
			// Otherwise
			else {
			
				// Get resistance value
				adc_write_configuration(&HEATER_READ_ADC, &resistanceReadAdcController);
				adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &resistanceReadAdcChannel);
			
				uint32_t resistanceValue = 0;
				for(uint8_t i = 0; i < 50; i++) {
					adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
					adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
					resistanceValue += adc_get_unsigned_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
				}
				resistanceValue /= 50;
			
				// Get heater temperature measurement B
				float heaterTemperatureMeasurementB;
				nvm_eeprom_read_buffer(EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_OFFSET, &heaterTemperatureMeasurementB, EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_LENGTH);
			
				// Get heater resistance M
				float heaterResistanceM;
				nvm_eeprom_read_buffer(EEPROM_HEATER_RESISTANCE_M_OFFSET, &heaterResistanceM, EEPROM_HEATER_RESISTANCE_M_LENGTH);
			
				// Check which heater calibration mode was used
				switch(nvm_eeprom_read_byte(EEPROM_HEATER_CALIBRATION_MODE_OFFSET)) {
			
					// Update actual temperature
					default:
						actualTemperature = 2.177778 * heaterValue / resistanceValue * heaterResistanceM + heaterTemperatureMeasurementB;
				}
			}
			
			// Check if temperature has been reached
			if(actualTemperature >= idealTemperature)
			
				// Turn heater off
				ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
		}
		
		// Otherwise
		else {
		
			// Clear actual temperature
			actualTemperature = 0;
		
			// Turn heater off
			ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
		}
	});
	tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_DIV1024_gc);
}

void Heater::setTemperature(uint16_t value, bool wait) {
	
	// Check if heating
	if((idealTemperature = value)) {
	
		// Set if newer temperature is lower
		bool lowerNewValue = value < getTemperature();
	
		// Turn on/off heater depending on new temperature
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, lowerNewValue ? HEATER_OFF : HEATER_ON);
		
		// Wait until temperature has been reached
		while(wait && ioport_get_pin_level(HEATER_MODE_SELECT_PIN) == (lowerNewValue ? HEATER_OFF : HEATER_ON)) {
		
			// Delay one second
			tc_restart(&TEMPERATURE_TIMER);
			for(temperatureIntervalCounter = 0; temperatureIntervalCounter < UPDATE_TEMPERATURE_PER_SECOND && !emergencyStopOccured && heaterWorking; delay_us(1));
			
			// Break if an emergency stop occured or heater isn't working
			if(emergencyStopOccured || !heaterWorking)
				break;
		
			// Set response to temperature
			char buffer[sizeof("4294967296") + NUMBER_OF_DECIMAL_PLACES + 4];
			strcpy(buffer, "T:");
			ftoa(getTemperature(), &buffer[2]);
			strcat(buffer, "\n");
		
			// Send temperature
			sendDataToUsb(buffer, true);
		}
	}
	
	// Otherwise
	else
	
		// Clear temperature
		clearTemperature();
}

float Heater::getTemperature() const {

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);

	// Get actual temperature
	float value = actualTemperature;
	
	// Allow updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	
	// Return value
	return value;
}

bool Heater::isWorking() {

	// Return if heater is working
	return heaterWorking;
}

void Heater::clearTemperature() {

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);

	// Clear ideal and actual temperature
	idealTemperature = actualTemperature = 0;
	
	// Allow updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);

	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
}

void Heater::reset() {

	// Clear temperature
	clearTemperature();
	
	// Clear mergency stop occured
	emergencyStopOccured = false;
}
