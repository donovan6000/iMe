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
#define UPDATE_TEMPERATURE_INTERVAL 0.5

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
uint8_t temperatureIntervalCounter = 0;
float idealTemperature = 0;
float actualTemperature = 0;
adc_config heaterReadAdcController;
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
	
	// Set ADC controller to use signed, 12bit, Vref refrence, manual trigger, 200kHz frequency
	adc_read_configuration(&HEATER_READ_ADC, &heaterReadAdcController);
	adc_set_conversion_parameters(&heaterReadAdcController, ADC_SIGN_ON, ADC_RES_12, ADC_REF_AREFA);
	adc_set_conversion_trigger(&heaterReadAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&heaterReadAdcController, 200000);
	
	// Set ADC channel to use heater read pins as a differential input
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	adcch_set_input(&heaterReadAdcChannel, HEATER_READ_POSITIVE_INPUT, HEATER_READ_NEGATIVE_INPUT, 1);
	
	// Configure update temperature timer
	tc_enable(&TEMPERATURE_TIMER);
	tc_set_wgm(&TEMPERATURE_TIMER, TC_WG_NORMAL);
	tc_write_period(&TEMPERATURE_TIMER, sysclk_get_cpu_hz() / 1024 * UPDATE_TEMPERATURE_INTERVAL);
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	tc_set_overflow_interrupt_callback(&TEMPERATURE_TIMER, []() -> void {
	
		// Increment temperature interval counter
		temperatureIntervalCounter++;
	
		// Check if setting the temperature
		if(idealTemperature) {
	
			// Turn on heater
			ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
	
			// Get heater temperature
			adc_write_configuration(&HEATER_READ_ADC, &heaterReadAdcController);
			adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
			adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
			adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
			int16_t value = adc_get_signed_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
			
			// Get heater calibration mode
			uint8_t heaterCalibrationMode = nvm_eeprom_read_byte(EEPROM_HEATER_CALIBRATION_MODE_OFFSET);
			
			// Get heater temperature measurement B
			float heaterTemperatureMeasurementB;
			nvm_eeprom_read_buffer(EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_OFFSET, &heaterTemperatureMeasurementB, EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_LENGTH);
			
			// Get heater resistance M
			float heaterResistanceM;
			nvm_eeprom_read_buffer(EEPROM_HEATER_RESISTANCE_M_OFFSET, &heaterResistanceM, EEPROM_HEATER_RESISTANCE_M_LENGTH);
			
			// Check which heater calibration mode was used
			switch(heaterCalibrationMode) {
			
				default:
				
					// TODO Update actual temperature
					actualTemperature = value * 0;
			}
			
			// Check if temperature has been reached
			if(actualTemperature > idealTemperature)
			
				// Turn heater off
				ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
		}
		
		// Otherwise
		else
		
			// Turn heater off
			ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
	});
	tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_DIV1024_gc);
	
	// Clear emergency stop occured
	emergencyStopOccured = false;
}

void Heater::setTemperature(uint16_t value, bool wait) {
	
	// Check if heating
	if((idealTemperature = value)) {
	
		// Set if newer temperature is lower
		bool lowerNewValue = value < getTemperature();
	
		// Turn on/off heater depending on new temperature
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, lowerNewValue ? HEATER_OFF : HEATER_ON);
		
		// Wait until temperature has been reached
		while(wait && ioport_get_pin_level(HEATER_MODE_SELECT_PIN) == lowerNewValue ? HEATER_OFF : HEATER_ON) {
		
			// Delay one second
			tc_restart(&TEMPERATURE_TIMER);
			for(temperatureIntervalCounter = 0; temperatureIntervalCounter < 1 / UPDATE_TEMPERATURE_INTERVAL && !emergencyStopOccured;)
				delay_us(1);
			
			// Break if an emergency stop occured
			if(emergencyStopOccured)
				break;
		
			// Set response to temperature
			char buffer[sizeof("4294967296") + NUMBER_OF_DECIMAL_PLACES + 4];
			strcpy(buffer, "T:");
			ftoa(getTemperature(), &buffer[2]);
			strcat(buffer, "\n");
		
			// Send temperature
			udi_cdc_write_buf(buffer, strlen(buffer));
		}
		
		// Clear ideal temperature if an emergency stop occured
		if(emergencyStopOccured)
			idealTemperature = 0;
	}
	
	// Otherwise
	else
	
		// Turn off heater
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
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

void Heater::emergencyStop() {

	// Clear ideal and actual temperature
	idealTemperature = actualTemperature = 0;

	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
	
	// Set Emergency stop occured
	emergencyStopOccured = true;
}
