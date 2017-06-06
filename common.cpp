// Header files
extern "C" {
	#include <asf.h>
}
#include <ctype.h>
#include <float.h>
#include <string.h>
#include "common.h"


// Supporting function implementation
void ulltoa(uint64_t value, char *buffer) {

	// Initialize variables
	uint8_t i = INT_BUFFER_SIZE - sizeof('-') - 1;
	
	// Add terminating character
	buffer[i] = 0;
	
	// Go through all digits
	do {

		// Set digit in buffer
		buffer[--i] = value % 10 + 0x30;
		value /= 10;
	} while(value);
	
	// Move string to the start of the buffer
	memmove(buffer, &buffer[i], INT_BUFFER_SIZE - i - sizeof('-'));
}

void lltoa(int64_t value, char *buffer) {

	// Convert value to string
	if(value < 0) {
		buffer[0] = '-';
		ulltoa(value * -1, buffer + sizeof('-'));
	}
	else
		ulltoa(value, buffer);
}

void ftoa(float value, char *buffer) {

	// Initialize variables
	uint8_t i = FLOAT_BUFFER_SIZE - 1;
	bool negative = value < 0;
	
	// Add terminating character
	buffer[i] = 0;
	
	// Add decimal character
	i -= sizeof('.') + NUMBER_OF_DECIMAL_PLACES;
	buffer[i] = '.';
	uint8_t j = i;
	
	// Make value positive
	value = fabs(value);
	
	// Go through all digits
	uint32_t temp = value;
	value -= temp;
	do {

		// Set digit in buffer
		buffer[--i] = temp % 10 + 0x30;
		temp /= 10;
	} while(temp);
	
	// Go through all decimals
	temp = value * pow(10, NUMBER_OF_DECIMAL_PLACES);
	for(uint8_t k = NUMBER_OF_DECIMAL_PLACES; k; k--) {
	
		// Set decimal digit in buffer
		buffer[j + k] = temp % 10 + 0x30;
		temp /= 10;
	}
	
	// Prepend minus sign if value is negative
	if(negative)
		buffer[--i] = '-';
	
	// Move string to the start of the buffer
	memmove(buffer, &buffer[i], FLOAT_BUFFER_SIZE - i);
}

uint64_t strtoull(const char *nptr, char **endptr) {

	// Initialize variables
	uint64_t value = 0;
	
	// Skip plus sign
	if(*nptr == '+' && isdigit(nptr[1]))
		nptr++;
	
	// Go through all characters
	for(; isdigit(*nptr); nptr++) {
	
		// Set digit in value
		value *= 10;
		value += *nptr - 0x30;
	}
	
	// Set end pointer to last valid character
	if(endptr)
		*endptr = const_cast<char *>(nptr);
	
	// Return value
	return value;
}

int64_t strtoll(const char *nptr, char **endptr) {

	// Return value converted to a long long
	return *nptr == '-' && isdigit(nptr[1]) ? strtoull(nptr + sizeof('-'), endptr) * -1 : strtoull(nptr, endptr);
}

float strtof(const char *nptr, char **endptr) {

	// Initialize variables
	float value;
	bool negative = false;
	
	// Check if value contains a sign
	if((*nptr == '-' || *nptr == '+') && (isdigit(nptr[1]) || (nptr[1] == '.' && isdigit(nptr[2])))) {
	
		// Set negative
		negative = *nptr == '-';
		
		// Skip sign
		nptr++;
	}
	
	// Get the integer and fractional part of the value
	for(bool firstPass = true;; firstPass = false) {
	
		// Initialize variables
		float *currentValue;
		float decimalValue;
		
		// Check if getting integer part of the value
		if(firstPass)
		
			// Set current value
			currentValue = &value;
		
		// Otherwise
		else {
		
			// Set current value
			currentValue = &decimalValue;
		
			// Check if a value contains a fractional part
			if(*nptr == '.' && isdigit(nptr[1]))

				// Move to first decimal digit
				nptr++;
		}
		
		// Clear current value
		*currentValue = 0;
	
		// Go through all characters
		uint8_t numberOfDigits = 0;
		for(; isdigit(*nptr); nptr++)
	
			// Check if value is valid
			if(firstPass || numberOfDigits != FLT_DIG) {
			
				// Set digit in current value
				*currentValue *= 10;
				*currentValue += *nptr - 0x30;
				
				// Increment number of digits
				numberOfDigits++;
			}
		
		// Check if getting fractional part of the value
		if(!firstPass) {
		
			// Convert current value into a fractional value
			for(uint8_t i = 0; i < numberOfDigits; i++)
				decimalValue /= 10;
	
			// Append fractional value to integer value
			value += decimalValue;
			
			// Break
			break;
		}
	}
	
	// Set end pointer to last valid character
	if(endptr)
		*endptr = const_cast<char *>(nptr);
	
	// Return value
	return negative ? value * -1 : value;
}

void sendDataToUsb(const char *data, bool checkBufferSize) {

	// Check if data can be sent
	uint8_t length = strlen(data);
	if(!checkBufferSize || udi_cdc_get_free_tx_buffer() >= length)
	
		// Send data
		udi_cdc_write_buf(data, length);
}

float getValueInRange(float value, float minValue, float maxValue) {

	// Return value limited by range
	return min(maxValue, max(minValue, value));
}

uint32_t minimumOneCeil(float value) {

	// Return ceiling of value that is at least one
	return getValueInRange(ceil(value), 1, UINT32_MAX);
}

void leadingPadBuffer(char *buffer, uint8_t size, char padding) {

	// Check if buffer is smaller that the specified size
	uint8_t bufferSize = strlen(buffer);
	if(bufferSize < size) {
	
		// Shift buffer toward the end
		memmove(&buffer[size - bufferSize], buffer, bufferSize + 1);
		
		// Prepend padding to buffer
		memset(buffer, padding, size - bufferSize);
	}
}
