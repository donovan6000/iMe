// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include "common.h"
#include "gcode.h"


// Definitions
#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
	#define PARAMETER_ORDER PSTR("gmtspxyzfen")
#else
	#define PARAMETER_ORDER "gmtspxyzfen"
#endif


// Function prototypes

/*
Name: Is whitespace
Purpose: Returns if a provided character is whitespace
*/
inline bool isWhitespace(char value) noexcept;


// Supporting function implementation
bool isWhitespace(char value) noexcept {

	// Return if character is whitespace
	return value == ' ' || (value >= '\t' && value <= '\r');
}

void Gcode::parseCommand(const char *command) noexcept {

	// Set that command has been parsed
	isParsed = true;
	
	// Clear command parameters
	commandParameters = 0;

	// Remove leading whitespace
	const char *firstValidCharacter = command;
	for(; isWhitespace(*firstValidCharacter); ++firstValidCharacter);
	
	// Get last valid character
	const char *lastValidCharacter = firstValidCharacter;
	for(; *lastValidCharacter && *lastValidCharacter != ';' && *lastValidCharacter != '*' && *lastValidCharacter != '\n'; ++lastValidCharacter);
	
	// Remove trailing white space
	for(--lastValidCharacter; lastValidCharacter >= firstValidCharacter && isWhitespace(*lastValidCharacter); --lastValidCharacter);
	
	// Check if command is empty
	if(++lastValidCharacter != firstValidCharacter) {
	
		// Set start and stop parsing offsets
		uint8_t startParsingOffset = firstValidCharacter - command;
		uint8_t stopParsingOffset = lastValidCharacter - command;
	
		// Check if command is a host command
		if(*firstValidCharacter == '@') {
	
			// Check if host commands are allowed
			#if ALLOW_HOST_COMMANDS == true
		
				// Set command length
				uint8_t commandLength = min(static_cast<uint8_t>(stopParsingOffset - startParsingOffset - 1), sizeof(hostCommand) - 1);
	
				// Check if host command isn't empty
				if(commandLength) {
	
					// Save host command
					strncpy(hostCommand, firstValidCharacter + 1, commandLength);
					hostCommand[commandLength] = 0;
		
					// Set command parameters
					commandParameters |= PARAMETER_HOST_COMMAND_OFFSET;
				}
			#endif
		}
	
		// Otherwise
		else {
		
			// Go through each valid character in the command
			for(uint8_t i = startParsingOffset; i < stopParsingOffset; ++i) {
		
				// Check if character is a valid parameter
				#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
					const char *parameterIndex = strchr_P(PARAMETER_ORDER, lowerCase(command[i]));
				#else
					const char *parameterIndex = strchr(PARAMETER_ORDER, lowerCase(command[i]));
				#endif
				if(parameterIndex) {
			
					// Check if parameter hasn't been obtained yet
					#if STORE_CONSTANTS_IN_PROGRAM_SPACE == true
						gcodeParameterOffset parameterBit = 1 << (parameterIndex - reinterpret_cast<PGM_P>(pgm_read_ptr(PARAMETER_ORDER)));
					#else
						gcodeParameterOffset parameterBit = 1 << (parameterIndex - PARAMETER_ORDER);
					#endif
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
							default:
								valueN = strtoull(&command[++i], &lastParameterCharacter);
						}
				
						// Check if parameter exists
						if(lastParameterCharacter != &command[i]) {
				
							// Set command parameters
							commandParameters |= parameterBit;
					
							// Set index
							i = lastParameterCharacter - command;
						}
				
						// Decrement index
						--i;
					}
				}
			}
			
			// Check if command contains a checksum
			const char *checksumCharacter = strchr(lastValidCharacter, '*');
			if(checksumCharacter) {
	
				// Check if checksum exists
				char *lastChecksumCharacter;
				uint8_t providedChecksum = strtoull(++checksumCharacter, &lastChecksumCharacter);
				if(lastChecksumCharacter != checksumCharacter) {
		
					// Calculate checksum
					uint8_t calculatedChecksum = 0;
					for(uint8_t i = 0; command[i] != '*'; ++i)
						calculatedChecksum ^= command[i];
			
					// Set valid checksum
					if(calculatedChecksum == providedChecksum)
						commandParameters |= VALID_CHECKSUM_OFFSET;
				}
			}
		}
	}
}

void Gcode::clearCommand() noexcept {

	// Set values to defaults
	isParsed = false;
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
	
	#if ALLOW_HOST_COMMANDS == true
		*hostCommand = 0;
	#endif
}

bool Gcode::isEmpty() const noexcept {

	// Return if command hasn't been parsed
	return !isParsed;
}

bool Gcode::hasParameterG() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_G_OFFSET;
}

uint8_t Gcode::getParameterG() const noexcept {

	// Return parameter's value
	return valueG;
}

bool Gcode::hasParameterM() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_M_OFFSET;
}

uint16_t Gcode::getParameterM() const noexcept {

	// Return parameter's value
	return valueM;
}

bool Gcode::hasParameterT() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_T_OFFSET;
}

uint8_t Gcode::getParameterT() const noexcept {

	// Return parameter's value
	return valueT;
}

bool Gcode::hasParameterS() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_S_OFFSET;
}

int32_t Gcode::getParameterS() const noexcept {

	// Return parameter's value
	return valueS;
}

bool Gcode::hasParameterP() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_P_OFFSET;
}

int32_t Gcode::getParameterP() const noexcept {

	// Return parameter's value
	return valueP;
}

bool Gcode::hasParameterX() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_X_OFFSET;
}

float Gcode::getParameterX() const noexcept {

	// Return parameter's value
	return valueX;
}

bool Gcode::hasParameterY() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_Y_OFFSET;
}

float Gcode::getParameterY() const noexcept {

	// Return parameter's value
	return valueY;
}

bool Gcode::hasParameterZ() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_Z_OFFSET;
}

float Gcode::getParameterZ() const noexcept {

	// Return parameter's value
	return valueZ;
}

bool Gcode::hasParameterF() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_F_OFFSET;
}

float Gcode::getParameterF() const noexcept {

	// Return parameter's value
	return valueF;
}

bool Gcode::hasParameterE() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_E_OFFSET;
}

float Gcode::getParameterE() const noexcept {

	// Return parameter's value
	return valueE;
}

bool Gcode::hasParameterN() const noexcept {

	// Return is parameter is set
	return commandParameters & PARAMETER_N_OFFSET;
}

uint64_t Gcode::getParameterN() const noexcept {

	// Return parameter's value
	return valueN;
}

#if ALLOW_HOST_COMMANDS == true
	bool Gcode::hasHostCommand() const noexcept {

		// Return is host command is set
		return commandParameters & PARAMETER_HOST_COMMAND_OFFSET;
	}

	const char *Gcode::getHostCommand() const noexcept {

		// Return host command
		return hostCommand;
	}
#endif

bool Gcode::hasValidChecksum() const noexcept {

	// Return if checksum is valid
	return commandParameters & VALID_CHECKSUM_OFFSET;
}
