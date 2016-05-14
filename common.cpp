// Header files
extern "C" {
	#include <asf.h>
}
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "common.h"


// Supporting function implementation
void ulltoa(uint64_t value, char *buffer) {

	// Initialize variables
	uint8_t i = INT_BUFFER_SIZE;
	
	// Add terminating character
	buffer[i] = 0;
	
	// Go through all digits
	do {

		// Set digit in buffer
		buffer[--i] = value % 10 + 0x30;
		value /= 10;
	} while(value);
	
	// Move string to the start of the buffer
	memmove(buffer, &buffer[i], INT_BUFFER_SIZE - i + 1);
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
	uint8_t i = FLOAT_BUFFER_SIZE;
	
	// Add terminating character
	buffer[i] = 0;
	
	// Add decimal character
	i -= sizeof('.') + NUMBER_OF_DECIMAL_PLACES;
	buffer[i] = '.';
	uint8_t j = i;
	
	// Go through all digits
	uint32_t temp = static_cast<uint32_t>(fabs(value));
	do {

		// Set digit in buffer
		buffer[--i] = temp % 10 + 0x30;
		temp /= 10;
	} while(temp);
	
	// Go through all decimals
	temp = static_cast<uint32_t>(round(fabs(value) * pow(10, NUMBER_OF_DECIMAL_PLACES)));
	for(uint8_t k = NUMBER_OF_DECIMAL_PLACES; k; k--) {
	
		// Set decimal digit in buffer
		buffer[j + k] = temp % 10 + 0x30;
		temp /= 10;
	}
	
	// Prepend minus sign if value is negative
	if(value < 0)
		buffer[--i] = '-';
	
	// Move string to the start of the buffer
	memmove(buffer, &buffer[i], FLOAT_BUFFER_SIZE - i + 1);
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
	float value = 0;
	bool negative = false;
	
	// Check if value contains a sign
	if((*nptr == '-' || *nptr == '+') && (isdigit(nptr[1]) || (nptr[1] == '.' && isdigit(nptr[2])))) {
	
		// Set negative
		negative = *nptr == '-';
		
		// Skip sign
		nptr++;
	}
	
	// Go through all characters
	for(; isdigit(*nptr); nptr++) {
	
		// Set digit in value
		value *= 10;
		value += *nptr - 0x30;
	}
	
	// Check if value contains a decimal
	if(*nptr == '.' && isdigit(nptr[1])) {

		// Move to first decimal digit
		nptr++;
	
		// Go through all decimal values
		for(uint32_t i = 10; isdigit(*nptr); nptr++, i *= 10)

			// Set decimal digit in value
			value += static_cast<float>(*nptr - 0x30) / i;
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
