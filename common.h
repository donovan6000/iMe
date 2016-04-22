// Header gaurd
#ifndef COMMON_H
#define COMMON_H


// Header files
#include <stdint.h>


// Definitions
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define NUMBER_OF_DECIMAL_PLACES 4


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
void sendDataToUsb(const char *data);


#endif
