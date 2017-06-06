// Header guard
#ifndef COMMON_H
#define COMMON_H


// Header files
#include <math.h>


// Definitions
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define NUMBER_OF_DECIMAL_PLACES 4
#define INT_BUFFER_SIZE (sizeof("18446744073709551615") + sizeof('-'))
#define FLOAT_BUFFER_SIZE (sizeof("4294967296") + sizeof('-') + sizeof('.') + NUMBER_OF_DECIMAL_PLACES)

// Unit conversions
#define INCHES_TO_MILLIMETERS_SCALAR 25.4
#define MILLIMETERS_TO_INCHES_SCALAR (1 / INCHES_TO_MILLIMETERS_SCALAR)

// Integer limits
#define UINT12_MAX static_cast<uint16_t>(pow(2, 12) - 1)
#define INT12_MAX static_cast<int16_t>(pow(2, 12 - 1) - 1)

// ADC
#define ADC_MODULE ADCA
#define ADC_VREF_PIN IOPORT_CREATE_PIN(PORTA, 0)

// Voltages
#define MICROCONTROLLER_VOLTAGE 3.3
#define ADC_VREF_VOLTAGE 2.6


// Function prototypes

/*
Name: ulltoa
Purpose: Converts an unsigned long long to string
*/
void ulltoa(uint64_t value, char *buffer);

/*
Name: lltoa
Purpose: Converts a long long to string
*/
void lltoa(int64_t value, char *buffer);

/*
Name: ftoa
Purpose: Converts a float to a string
*/
void ftoa(float value, char *buffer);

/*
Name: strtoull
Purpose: Converts a string to an unsigned long long
*/
uint64_t strtoull(const char *nptr, char **endptr = nullptr);

/*
Name: strtoll
Purpose: Converts a string to a long long
*/
int64_t strtoll(const char *nptr, char **endptr = nullptr);

/*
Name: strtof
Purpose: Converts a string to a float
*/
float strtof(const char *nptr, char **endptr = nullptr);

/*
Name: Send data to USB
Purpose: Sends data to the USB host
*/
void sendDataToUsb(const char *data, bool checkBufferSize = false);

/*
Name: Get value in range
Purpose: Returns a value limited by the ranges min and max
*/
float getValueInRange(float value, float minValue, float maxValue);

/*
Name: Minimum one ceil
Purpose: Returns the ceiling of the value that is at least one
*/
uint32_t minimumOneCeil(float value);

/*
Name: Leading pad buffer
Purpose: Adds leading padding to buffer to make it meet the size specified
*/
void leadingPadBuffer(char *buffer, uint8_t size = 2, char padding = '0');


#endif
