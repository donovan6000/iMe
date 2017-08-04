// Header guard
#ifndef HEATER_H
#define HEATER_H


// Definitions
#define TEMPERATURE_TIMER TCC1
#define HEATER_OFF_TEMPERATURE 0
#define HEATER_MIN_TEMPERATURE 100
#define HEATER_MAX_TEMPERATURE 285


// Heater class
class Heater final {

	// Public
	public:
	
		// Initialize
		static void initialize() noexcept;
		
		// Test connection
		static bool testConnection() noexcept;
		
		// Set temperature
		static bool setTemperature(uint16_t value, bool wait = false) noexcept;
		
		// Get temperature
		static float getTemperature() noexcept;
		
		// Reset
		static void reset() noexcept;
		
		// Is on
		static bool isOn() noexcept;
		
		// Update heater changes
		static bool updateHeaterChanges(bool enableUpdatingTemperature = true) noexcept;
		
		// Is working
		static bool isWorking;
	
	// Private
	private:
	
		// Clear temperature
		static void clearTemperature() noexcept;
};


#endif
