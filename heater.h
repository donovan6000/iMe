// Header guard
#ifndef HEATER_H
#define HEATER_H


// Definitions
#define TEMPERATURE_TIMER TCC1
#define HEATER_OFF_TEMPERATURE 0
#define HEATER_MIN_TEMPERATURE 100
#define HEATER_MAX_TEMPERATURE 285


// Heater class
class Heater {

	// Public
	public:
	
		// Initialize
		static void initialize();
		
		// Test connection
		static bool testConnection();
		
		// Set temperature
		static bool setTemperature(uint16_t value, bool wait = false);
		
		// Get temperature
		static float getTemperature();
		
		// Is heating
		static bool isHeating();
		
		// Reset
		static void reset();
		
		// Update heater changes
		static bool updateHeaterChanges(bool enableUpdatingTemperature = true);
		
		// Emergency stop occured
		static bool emergencyStopOccured;
		
		// Is working
		static bool isWorking;
	
	// Private
	private:
	
		// Clear temperature
		static void clearTemperature();
};


#endif
