// Header files
#include <string.h>
#include <ctype.h>
#include "gcode.h"
#include "common.h"


// Definitions
#define PARAMETER_ORDER "GMTSPXYZFEN"


// Supporting function implementation
Gcode::Gcode() {

	// Clear command
	clearCommand();
}

bool Gcode::parseCommand(const char *command) {

	// Clear command
	clearCommand();

	// Remove leading whitespace
	const char *firstValidCharacter = command;
	for(;isspace(*firstValidCharacter); firstValidCharacter++);
	
	// Get last valid character
	const char *lastValidCharacter = firstValidCharacter;
	for(; *lastValidCharacter && *lastValidCharacter != ';' && *lastValidCharacter != '*'; lastValidCharacter++);
	
	// Remove trailing white space
	for(lastValidCharacter--; (lastValidCharacter >= firstValidCharacter) && isspace(*lastValidCharacter); lastValidCharacter--);
	
	// Check if command is empty
	if(++lastValidCharacter == firstValidCharacter)
	
		// Return false
		return false;
	
	// Set start and stop parsing offsets
	uint8_t startParsingOffset = firstValidCharacter - command;
	uint8_t stopParsingOffset = lastValidCharacter - command;
	
	// Check if command is a host command
	if(*firstValidCharacter == '@') {
	
		// Set command length
		uint8_t commandLength = stopParsingOffset - startParsingOffset - 1;
	
		// Check if host command is empty
		if(!commandLength)
		
			// Return false
			return false;
	
		// Save host command
		strncpy(hostCommand, firstValidCharacter + 1, commandLength);
		hostCommand[commandLength] = 0;
		
		// Set command parameters
		commandParameters |= PARAMETER_HOST_COMMAND_OFFSET;
	}
	
	// Otherwise
	else
	
		// Go through each valid character in the command
		for(uint8_t i = startParsingOffset; i < stopParsingOffset; i++) {
		
			// Get parameter index
			const char *parameterIndex = strchr(PARAMETER_ORDER, toupper(command[i]));
	
			// Check if character is a valid parameter
			if(parameterIndex) {
			
				// Check if parameter hasn't been obtained yet
				uint16_t parameterBit = 1 << (parameterIndex - PARAMETER_ORDER);
				if(!(commandParameters & parameterBit)) {
			
					// Save parameter value
					char *lastParameterCharacter;
					switch(parameterBit) {
			
						case PARAMETER_G_OFFSET:
							valueG = strtoull(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_M_OFFSET:
							valueM = strtoull(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_T_OFFSET:
							valueT = strtoull(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_S_OFFSET:
							valueS = strtoll(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_P_OFFSET:
							valueP = strtoll(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_X_OFFSET:
							valueX = strtof(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_Y_OFFSET:
							valueY = strtof(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_Z_OFFSET:
							valueZ = strtof(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_F_OFFSET:
							valueF = strtof(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_E_OFFSET:
							valueE = strtof(&command[++i], &lastParameterCharacter);
						break;
				
						case PARAMETER_N_OFFSET:
							valueN = strtoull(&command[++i], &lastParameterCharacter);
						
							// Check if command contains a checksum
							const char *checksumCharacter;
							if((checksumCharacter = strchr(command, '*'))) {
						
								// Check if checksum exists
								char *lastChecksumCharacter;
								uint8_t providedChecksum = strtoull(++checksumCharacter, &lastChecksumCharacter);
								if(lastChecksumCharacter != checksumCharacter) {
							
									// Calculate checksum
									uint8_t calculatedChecksum = 0;
									for(uint8_t i = 0; command[i] != '*'; i++)
										calculatedChecksum ^= command[i];
								
									// Set valid checksum
									if(calculatedChecksum == providedChecksum)
										commandParameters |= VALID_CHECKSUM_OFFSET;
								}
							}
					}
				
					// Check if parameter exists
					if(lastParameterCharacter != &command[i]) {
				
						// Set command parameters
						commandParameters |= parameterBit;
					
						// Set index
						i = lastParameterCharacter - command;
					}
				
					// Decrement index
					i--;
				}
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
	*hostCommand = 0;
}

bool Gcode::isEmpty() const {

	// Return if command contains no parameters
	return !commandParameters;
}

bool Gcode::hasParameterG() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_G_OFFSET;
}

uint8_t Gcode::getParameterG() const {

	// Return parameter's value
	return valueG;
}

bool Gcode::hasParameterM() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_M_OFFSET;
}

uint16_t Gcode::getParameterM() const {

	// Return parameter's value
	return valueM;
}

bool Gcode::hasParameterT() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_T_OFFSET;
}

uint8_t Gcode::getParameterT() const {

	// Return parameter's value
	return valueT;
}

bool Gcode::hasParameterS() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_S_OFFSET;
}

int32_t Gcode::getParameterS() const {

	// Return parameter's value
	return valueS;
}

bool Gcode::hasParameterP() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_P_OFFSET;
}

int32_t Gcode::getParameterP() const {

	// Return parameter's value
	return valueP;
}

bool Gcode::hasParameterX() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_X_OFFSET;
}

float Gcode::getParameterX() const {

	// Return parameter's value
	return valueX;
}

bool Gcode::hasParameterY() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_Y_OFFSET;
}

float Gcode::getParameterY() const {

	// Return parameter's value
	return valueY;
}

bool Gcode::hasParameterZ() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_Z_OFFSET;
}

float Gcode::getParameterZ() const {

	// Return parameter's value
	return valueZ;
}

bool Gcode::hasParameterF() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_F_OFFSET;
}

float Gcode::getParameterF() const {

	// Return parameter's value
	return valueF;
}

bool Gcode::hasParameterE() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_E_OFFSET;
}

float Gcode::getParameterE() const {

	// Return parameter's value
	return valueE;
}

bool Gcode::hasParameterN() const {

	// Return is parameter is set
	return commandParameters & PARAMETER_N_OFFSET;
}

uint64_t Gcode::getParameterN() const {

	// Return parameter's value
	return valueN;
}

bool Gcode::hasHostCommand() const {

	// Return is host command is set
	return *hostCommand;
}

const char *Gcode::getHostCommand() const {

	// Return host command
	return hostCommand;
}

bool Gcode::hasValidChecksum() const {

	// Return if checksum is valid
	return commandParameters & VALID_CHECKSUM_OFFSET;
}
