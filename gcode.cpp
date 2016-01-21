// Header files
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "gcode.h"


// Definitions
#define PARAMETER_ORDER "GMTSPXYZFEN"
#define PARAMETER_G_OFFSET 0x1 
#define PARAMETER_M_OFFSET 0x2
#define PARAMETER_T_OFFSET 0x4 
#define PARAMETER_S_OFFSET 0x8
#define PARAMETER_P_OFFSET 0x10
#define PARAMETER_X_OFFSET 0x20
#define PARAMETER_Y_OFFSET 0x40
#define PARAMETER_Z_OFFSET 0x80
#define PARAMETER_F_OFFSET 0x100
#define PARAMETER_E_OFFSET 0x200
#define PARAMETER_N_OFFSET 0x400


// Supporting function implementation
Gcode::Gcode(const char *command) {

	// Check if a command is specified
	if(command != NULL)
	
		// Parse command
		parseCommand(command);
	
	// Otherwise
	else
	
		// Clear command
		clearCommand();
}

bool Gcode::parseCommand(const char *command) {

	// Initialize variables
	const char *parameterIndex;
	uint16_t parameterOffset;
	char *lastParameterCharacter;
	uint8_t stopParsingOffset;
	char *lastValidCharacter;

	// Clear command
	clearCommand();
	
	// Check if command contains a comment or newline character
	lastValidCharacter = strchr(const_cast<char *>(command), '*');
	
	if(lastValidCharacter == NULL)
		lastValidCharacter = strchr(const_cast<char *>(command), '\n');
	
	if(lastValidCharacter != NULL)
	
		// Set stop parsing offset at comment or newline
		stopParsingOffset = lastValidCharacter - command;
	
	// Otherwise
	else
	
		// Set stop parsing offset at end of command
		stopParsingOffset = strlen(command);
	
	// Go through each character in the command
	for(uint8_t i = 0; i < stopParsingOffset; i++) {
	
		// Check if character is a valid parameter
		if(isalpha(command[i]) && (parameterIndex = strchr(PARAMETER_ORDER, command[i])) != NULL) {
		
			// Get parameter's offset
			parameterOffset = 1 << (parameterIndex - PARAMETER_ORDER);
			
			// Go through parameter's value
			lastParameterCharacter = const_cast<char *>(&command[++i]);
			for(uint8_t j = i; j < stopParsingOffset; j++)
			
				// Check if at end of parameter's value
				if(isspace(command[j]) || isalpha(command[j])) {
				
					// Save offset
					lastParameterCharacter = const_cast<char *>(&command[j]);
					break;
				}
				
				// Otherwise check if parameter goes to the end of the command
				else if(j == stopParsingOffset - 1) {
				
					// Set offset
					lastParameterCharacter = NULL;
					break;
				}
			
			// Check if parameter exists
			if(lastParameterCharacter != &command[i]) {
			
				// Set command parameters
				commandParameters |= parameterOffset;
				
				// Save parameter's value
				switch(parameterOffset) {
				
					case PARAMETER_G_OFFSET :
						valueG = strtol(&command[i], reinterpret_cast<char**>(&lastParameterCharacter), 10); 
					break;
					
					case PARAMETER_M_OFFSET :
						valueM = strtol(&command[i], reinterpret_cast<char**>(&lastParameterCharacter), 10); 
					break;
					
					case PARAMETER_T_OFFSET :
						valueT = strtol(&command[i], reinterpret_cast<char**>(&lastParameterCharacter), 10); 
					break;
					
					case PARAMETER_S_OFFSET :
						valueS = strtol(&command[i], reinterpret_cast<char**>(&lastParameterCharacter), 10); 
					break;
					
					case PARAMETER_P_OFFSET :
						valueP = strtol(&command[i], reinterpret_cast<char**>(&lastParameterCharacter), 10); 
					break;
					
					case PARAMETER_X_OFFSET :
						valueX = strtod(&command[i], reinterpret_cast<char**>(&lastParameterCharacter));
					break;
					
					case PARAMETER_Y_OFFSET :
						valueY = strtod(&command[i], reinterpret_cast<char**>(&lastParameterCharacter));
					break;
					
					case PARAMETER_Z_OFFSET :
						valueZ = strtod(&command[i], reinterpret_cast<char**>(&lastParameterCharacter));
					break;
					
					case PARAMETER_F_OFFSET :
						valueF = strtod(&command[i], reinterpret_cast<char**>(&lastParameterCharacter));
					break;
					
					case PARAMETER_E_OFFSET :
						valueE = strtod(&command[i], reinterpret_cast<char**>(&lastParameterCharacter));
					break;
					
					case PARAMETER_N_OFFSET :
						valueN = strtoul(&command[i], reinterpret_cast<char**>(&lastParameterCharacter), 10); 
					break;
				}
				
				// Check if parameter's value went to the end of the command
				if(lastParameterCharacter == NULL || lastParameterCharacter - command >= stopParsingOffset)
				
					// Break
					break;
				
				// Otherwise
				else
				
					// Increment index
					i += lastParameterCharacter - &command[i];
			}
			
			// Decrement index
			i--;
		}
	}
	
	// Return if command contains parameters
	return commandParameters;
}

void Gcode::clearCommand() {

	// Set values to defaults
	commandParameters = 0;
	valueG = 0;
	valueM = 0;
	valueT = 0;
	valueS = 0;
	valueP = 0;
	valueX = 0;
	valueY = 0;
	valueZ = 0;
	valueF = 0;
	valueE = 0;
	valueN = 0;
}

bool Gcode::isEmpty() {

	// Return if command contains no parameters
	return !commandParameters;
}

bool Gcode::hasParameterG() {

	// Return is parameter is set
	return commandParameters & PARAMETER_G_OFFSET;
}

uint8_t Gcode::getParameterG() {

	// Return parameter's value
	return valueG;
}

bool Gcode::hasParameterM() {

	// Return is parameter is set
	return commandParameters & PARAMETER_M_OFFSET;
}

uint8_t Gcode::getParameterM() {

	// Return parameter's value
	return valueM;
}

bool Gcode::hasParameterT() {

	// Return is parameter is set
	return commandParameters & PARAMETER_T_OFFSET;
}

int8_t Gcode::getParameterT() {

	// Return parameter's value
	return valueT;
}

bool Gcode::hasParameterS() {

	// Return is parameter is set
	return commandParameters & PARAMETER_S_OFFSET;
}

int32_t Gcode::getParameterS() {

	// Return parameter's value
	return valueS;
}

bool Gcode::hasParameterP() {

	// Return is parameter is set
	return commandParameters & PARAMETER_P_OFFSET;
}

int32_t Gcode::getParameterP() {

	// Return parameter's value
	return valueP;
}

bool Gcode::hasParameterX() {

	// Return is parameter is set
	return commandParameters & PARAMETER_X_OFFSET;
}

float Gcode::getParameterX() {

	// Return parameter's value
	return valueX;
}

bool Gcode::hasParameterY() {

	// Return is parameter is set
	return commandParameters & PARAMETER_Y_OFFSET;
}

float Gcode::getParameterY() {

	// Return parameter's value
	return valueY;
}

bool Gcode::hasParameterZ() {

	// Return is parameter is set
	return commandParameters & PARAMETER_Z_OFFSET;
}

float Gcode::getParameterZ() {

	// Return parameter's value
	return valueZ;
}

bool Gcode::hasParameterF() {

	// Return is parameter is set
	return commandParameters & PARAMETER_F_OFFSET;
}

float Gcode::getParameterF() {

	// Return parameter's value
	return valueF;
}

bool Gcode::hasParameterE() {

	// Return is parameter is set
	return commandParameters & PARAMETER_E_OFFSET;
}

float Gcode::getParameterE() {

	// Return parameter's value
	return valueE;
}

bool Gcode::hasParameterN() {

	// Return is parameter is set
	return commandParameters & PARAMETER_N_OFFSET;
}

uint64_t Gcode::getParameterN() {

	// Return parameter's value
	return valueN;
}
