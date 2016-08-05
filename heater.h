// Header gaurd
#ifndef HEATER_H
#define HEATER_H


// Definitions
#define TEMPERATURE_TIMER TCC1
#define HEATER_MIN_TEMPERATURE 100
#define HEATER_MAX_TEMPERATURE 285


// Heater class
class Heater {

	// Public
	public:
	
		// Initialize
		void initialize();
		
		// Set temperature
		void setTemperature(uint16_t value, bool wait);
		
		// Get temperature
		float getTemperature() const;
		
		// Reset
		void reset();
		
		// Emergency stop occured
		bool emergencyStopOccured;
		
		// Is working
		bool isWorking;
	
	// Private
	private:
	
		// Clear temperature
		inline void clearTemperature();
};


#endif
