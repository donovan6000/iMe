// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include "common.h"
#include "eeprom.h"
#include "heater.h"


// Definitions
#define UPDATE_TEMPERATURE_MILLISECONDS 333
#define HEATER_READ_ADC ADC_MODULE
#define HEATER_READ_ADC_FREQUENCY 200000
#define HEATER_READ_ADC_SAMPLE_SIZE 50
#define IDEAL_HEATER_RESISTANCE_M 245.0
#define IDEAL_HEATER_TEMPERATURE_MEASUREMENT_B -450.0
#define HEATER_VOLTAGE_TO_TEMPERATURE_SCALAR (-IDEAL_HEATER_RESISTANCE_M / IDEAL_HEATER_TEMPERATURE_MEASUREMENT_B * 4)
#define INVALID_HEATER_CALIBRATION_MODE 0
#define MAX_SUPPORTED_HEATER_CALIBRATION_MODE 1

// Heater Pins
#define HEATER_MODE_SELECT_PIN IOPORT_CREATE_PIN(PORTE, 2)
#define HEATER_ENABLE_PIN IOPORT_CREATE_PIN(PORTA, 2)
#define HEATER_READ_POSITIVE_PIN IOPORT_CREATE_PIN(PORTA, 3)
#define HEATER_READ_NEGATIVE_PIN IOPORT_CREATE_PIN(PORTA, 4)
#define RESISTANCE_READ_PIN IOPORT_CREATE_PIN(PORTA, 5)

// Heater ADC
#define HEATER_READ_ADC_CHANNEL ADC_CH0
#define HEATER_READ_POSITIVE_INPUT ADCCH_POS_PIN3
#define HEATER_READ_NEGATIVE_INPUT ADCCH_NEG_PIN4
#define RESISTANCE_READ_INPUT ADCCH_POS_PIN5

// Pin states
#define HEATER_ON IOPORT_PIN_LEVEL_HIGH
#define HEATER_OFF IOPORT_PIN_LEVEL_LOW
#define HEATER_ENABLE IOPORT_PIN_LEVEL_HIGH
#define HEATER_DISABLE IOPORT_PIN_LEVEL_LOW


// Static class variables
bool Heater::isWorking;


// Global variables
float idealTemperature;
float actualTemperature;
adc_config heaterReadAdcController;
adc_channel_config heaterReadAdcChannel;
adc_config resistanceReadAdcController;
adc_channel_config resistanceReadAdcChannel;
uint8_t heaterCalibrationMode;
float heaterTemperatureMeasurementB;
float heaterResistanceM;


// Function prototypes

/*
Name: Get heater value
Purpose: Returns heater's ADC value
*/
int16_t getHeaterValue(bool testConnection) noexcept;


// Supporting function implementation
int16_t getHeaterValue(bool testConnection) noexcept {

	// Get heater value
	adc_write_configuration(&HEATER_READ_ADC, &heaterReadAdcController);
	adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	
	int32_t heaterValue = 0;
	uint8_t sampleSize = testConnection ? 1 : HEATER_READ_ADC_SAMPLE_SIZE;
	for(uint8_t i = sampleSize; i > 0; --i) {
		adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
		adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
		heaterValue += adc_get_signed_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
	}
	
	// Return heater value
	return heaterValue / sampleSize;
}

void Heater::initialize() noexcept {

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
	
	// Set ADC heater controller to use signed, 12-bit, bandgap refrence, and manual trigger
	adc_read_configuration(&HEATER_READ_ADC, &heaterReadAdcController);
	adc_set_conversion_parameters(&heaterReadAdcController, ADC_SIGN_ON, ADC_RES_12, ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&heaterReadAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&heaterReadAdcController, HEATER_READ_ADC_FREQUENCY);
	
	// Set ADC heater channel to use heater read pins as a differential input with 1/2x gain
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &heaterReadAdcChannel);
	adcch_set_input(&heaterReadAdcChannel, HEATER_READ_POSITIVE_INPUT, HEATER_READ_NEGATIVE_INPUT, 0);
	
	// Set ADC resistance controller to use unsigned, 12-bit, bandgap refrence, and manual trigger
	adc_read_configuration(&HEATER_READ_ADC, &resistanceReadAdcController);
	adc_set_conversion_parameters(&resistanceReadAdcController, ADC_SIGN_OFF, ADC_RES_12, ADC_REF_BANDGAP);
	adc_set_conversion_trigger(&resistanceReadAdcController, ADC_TRIG_MANUAL, ADC_NR_OF_CHANNELS, 0);
	adc_set_clock_rate(&resistanceReadAdcController, HEATER_READ_ADC_FREQUENCY);
	
	// Set ADC resistance channel to use resistance read pin as a single input with no gain
	adcch_read_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &resistanceReadAdcChannel);
	adcch_set_input(&resistanceReadAdcChannel, RESISTANCE_READ_INPUT, ADCCH_NEG_NONE, 1);
	
	// Configure update temperature timer
	tc_enable(&TEMPERATURE_TIMER);
	tc_set_wgm(&TEMPERATURE_TIMER, TC_WG_NORMAL);
	tc_write_period(&TEMPERATURE_TIMER, sysclk_get_cpu_hz() / 1024 * UPDATE_TEMPERATURE_MILLISECONDS / 1000);
	tc_set_overflow_interrupt_callback(&TEMPERATURE_TIMER, []() noexcept -> void {
		
		// Check if setting the temperature and heater is working
		if(idealTemperature != HEATER_OFF_TEMPERATURE && testConnection()) {
	
			// Turn on heater
			ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_ON);
			
			// Wait enough time for heater voltage to stabilize
			delayMicroseconds(500);
	
			// Get resistance value
			adc_write_configuration(&HEATER_READ_ADC, &resistanceReadAdcController);
			adcch_write_configuration(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL, &resistanceReadAdcChannel);
		
			uint32_t resistanceValue = 0;
			for(uint8_t i = 0; i < HEATER_READ_ADC_SAMPLE_SIZE; ++i) {
				adc_start_conversion(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
				adc_wait_for_interrupt_flag(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
				resistanceValue += adc_get_unsigned_result(&HEATER_READ_ADC, HEATER_READ_ADC_CHANNEL);
			}
			resistanceValue /= HEATER_READ_ADC_SAMPLE_SIZE;
		
			// Check which heater calibration mode was used
			switch(heaterCalibrationMode) {
		
				// Heater calibration mode 1
				case 1:
				default:
				
					// Update actual temperature
					actualTemperature = HEATER_VOLTAGE_TO_TEMPERATURE_SCALAR * getHeaterValue(false) / resistanceValue * heaterResistanceM + heaterTemperatureMeasurementB;
			}
			
			// Check if temperature has been reached
			if(actualTemperature >= idealTemperature)
			
				// Turn heater off
				ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
		}
		
		// Otherwise
		else
		
			// Clear temperature
			clearTemperature();
	});
	tc_write_clock_source(&TEMPERATURE_TIMER, TC_CLKSEL_DIV1024_gc);
}

bool Heater::testConnection() noexcept {

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);
	
	// Check if heater isn't working
	if(!(isWorking = getHeaterValue(true) < INT12_MAX / 2))
	
		// Clear temperature
		clearTemperature();
	
	// Allow updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	
	// Return if heater is working
	return isWorking;
}

bool Heater::setTemperature(uint16_t value, bool wait) noexcept {

	// Set if heater is working
	testConnection();

	// Check if heater calibration mode isn't supported
	if(!updateHeaterChanges(false))

		// Return false
		return false;
	
	// Check if heating
	if((idealTemperature = value) != HEATER_OFF_TEMPERATURE) {
	
		// Set if newer temperature is lower
		bool lowerNewValue = value < getTemperature();
	
		// Turn on or off heater depending on new temperature
		ioport_set_pin_level(HEATER_MODE_SELECT_PIN, lowerNewValue ? HEATER_OFF : HEATER_ON);
		
		// Wait until temperature has been reached
		while(wait) {
		
			// Break if an emergency stop occured or heater isn't working
			if(emergencyStopRequest || !isWorking)
				
				// Break
				break;
		
			// Set response to temperature
			char buffer[FLOAT_BUFFER_SIZE + sizeof("T:\n") - 1];
			#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
				strcpy_P(buffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("T:"))));
			#else
				strcpy(buffer, "T:");
			#endif
			
			// Prevent updating temperature
			tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);
			
			// Set if done heating
			bool doneHeating = ioport_get_pin_output_level(HEATER_MODE_SELECT_PIN) != (lowerNewValue ? HEATER_OFF : HEATER_ON);
			
			// Append correctly looking temperature to response
			ftoa(doneHeating && (lowerNewValue ? actualTemperature >= idealTemperature : actualTemperature <= idealTemperature) ? idealTemperature + (lowerNewValue ? -1 : 1) / pow(10, NUMBER_OF_DECIMAL_PLACES) : actualTemperature, &buffer[sizeof("T:") - 1]);
			
			// Allow updating temperature
			tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
			
			// Append newline to response
			#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
				strcat_P(buffer, reinterpret_cast<PGM_P>(pgm_read_ptr(PSTR("\n"))));
			#else
				strcat(buffer, "\n");
			#endif
		
			// Send temperature
			sendDataToUsb(buffer, true);
			
			// Check if done heating
			if(doneHeating)
			
				// Break
				break;
			
			// Delay one second or until heater stops working
			delayHundredsOfMicroseconds(1 * SECONDS_TO_HUNDREDS_OF_MICROSECONDS_SCALAR, &isWorking);
		}
	}
	
	// Otherwise
	else
	
		// Clear temperature
		clearTemperature();
	
	// Return true
	return true;
}

float Heater::getTemperature() noexcept {

	// Set if heater is working
	testConnection();

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);

	// Get actual temperature
	float value = actualTemperature;
	
	// Allow updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	
	// Return value
	return value;
}

void Heater::clearTemperature() noexcept {

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);

	// Clear ideal and actual temperature
	idealTemperature = actualTemperature = HEATER_OFF_TEMPERATURE;
	
	// Allow updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);

	// Turn off heater
	ioport_set_pin_level(HEATER_MODE_SELECT_PIN, HEATER_OFF);
}

bool Heater::updateHeaterChanges(bool enableUpdatingTemperature) noexcept {

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);

	// Get heater calibration mode
	heaterCalibrationMode = nvm_eeprom_read_byte(EEPROM_HEATER_CALIBRATION_MODE_OFFSET);
	
	// Get heater temperature measurement B
	nvm_eeprom_read_buffer(EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_OFFSET, &heaterTemperatureMeasurementB, EEPROM_HEATER_TEMPERATURE_MEASUREMENT_B_LENGTH);

	// Get heater resistance M
	nvm_eeprom_read_buffer(EEPROM_HEATER_RESISTANCE_M_OFFSET, &heaterResistanceM, EEPROM_HEATER_RESISTANCE_M_LENGTH);
	
	// Check if heater calibration mode isn't supported
	if(heaterCalibrationMode == INVALID_HEATER_CALIBRATION_MODE || heaterCalibrationMode > MAX_SUPPORTED_HEATER_CALIBRATION_MODE) {
	
		// Clear temperature
		clearTemperature();
		
		// Return false
		return false;
	}
	
	// Check if enabling updating temperature
	if(enableUpdatingTemperature)
	
		// Allow updating temperature
		tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	
	// Return true
	return true;
}

void Heater::reset() noexcept {

	// Clear temperature
	clearTemperature();
}

bool Heater::isOn() noexcept {

	// Set if heater is working
	testConnection();

	// Prevent updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_OFF);

	// Set if on
	bool on = idealTemperature != HEATER_OFF_TEMPERATURE;
	
	// Allow updating temperature
	tc_set_overflow_interrupt_level(&TEMPERATURE_TIMER, TC_INT_LVL_LO);
	
	// Return if heater is on
	return on;
}
