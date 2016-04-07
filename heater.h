// Header gaurd
#ifndef HEATER_H
#define HEATER_H


// Definitions
#define MIN_TEMPERATURE 100
#define MAX_TEMPERATURE 285


// Heater class
class Heater {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Set temperature
		void setTemperature(uint16_t value, bool wait);
		
		// Get temperature
		float getTemperature();
		
		// Emergency stop
		void emergencyStop();
	
	// Private
	private:
	
		// Heater read ADC channel
		adc_channel_config heaterReadAdcChannel;
};


#endif
