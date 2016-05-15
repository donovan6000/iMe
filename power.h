// Header gaurd
#ifndef POWER_H
#define POWER_H


// Power class
class Power {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Get supply voltage
		float getSupplyVoltage();
		
		// Stable supply voltage
		float stableSupplyVoltage;
	
	// Private
	private:
	
		// Voltage read ADC controller and channel
		adc_config voltageReadAdcController;
		adc_channel_config voltageReadAdcChannel;
};


#endif
