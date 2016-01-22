// Header gaurd
#ifndef GCODE_H
#define GCODE_H


// Header files
#include <stdint.h>


// Gcode class
class Gcode {

	// Public
	public :
	
		// Parameterized constructor
		Gcode(const char *command = NULL);
		
		// Parse command
		bool parseCommand(const char *command);
		
		// Clear command
		void clearCommand();
		
		// Is empty
		bool isEmpty();
		
		// Has parameter G
		bool hasParameterG();
		
		// Get parameter G
		uint8_t getParameterG();
		
		// Has parameter M
		bool hasParameterM();
		
		// Get parameter M
		uint16_t getParameterM();
		
		// Has parameter T
		bool hasParameterT();
		
		// Get parameter T
		int8_t getParameterT();
		
		// Has parameter S
		bool hasParameterS();
		
		// Get parameter S
		int32_t getParameterS();
		
		// Has parameter P
		bool hasParameterP();
		
		// Get parameter P
		int32_t getParameterP();
		
		// Has parameter X
		bool hasParameterX();
		
		// Get parameter X
		float getParameterX();
		
		// Has parameter Y
		bool hasParameterY();
		
		// Get parameter Y
		float getParameterY();
		
		// Has parameter Z
		bool hasParameterZ();
		
		// Get parameter Z
		float getParameterZ();
		
		// Has parameter F
		bool hasParameterF();
		
		// Get parameter F
		float getParameterF();
		
		// Has parameter E
		bool hasParameterE();
		
		// Get parameter E
		float getParameterE();
		
		// Has parameter N
		bool hasParameterN();
		
		// Get parameter N
		uint32_t getParameterN();
		
		// Has host command
		bool hasHostCommand();
		
		// Get host command
		char *getHostCommand();
	
	// Private
	private :
	
		// Command parameters
		uint16_t commandParameters;
		
		// Values
		uint8_t valueG;
		uint16_t valueM;
		int8_t valueT;
		int32_t valueS;
		int32_t valueP;
		float valueX;
		float valueY;
		float valueZ;
		float valueF;
		float valueE;
		uint32_t valueN;
		
		// Host command
		char hostCommand[64];
};


#endif
