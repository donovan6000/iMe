// Header gaurd
#ifndef COMMON_H
#define COMMON_H


// Header files
#include <inttypes.h>


// Definitions
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)


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
uint64_t strtoull(const char *nptr, char **endptr);

/*
Name: strtoll
Purpose: Converts a string to a long long
*/
int64_t strtoll(const char *nptr, char **endptr);

/*
Name: strtof
Purpose: Converts a string to a float
*/
float strtof(const char *nptr, char **endptr);


#endif
