// Header files
extern "C" {
	#include <asf.h>
}
#include <math.h>
#include <string.h>
#include "vector.h"


// Definitions
#define NUMBER_OF_COMPONENTS 4


// Supporting function implementation
void Vector::initialize(float x, float y, float z, float e) noexcept {

	// Set vector components
	this->x = x;
	this->y = y;
	this->z = z;
	this->e = e;
}

float Vector::getLength() const noexcept {

	// Get length squared
	float lengthSquared = 0;
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		lengthSquared += pow((*this)[i], 2);
	
	// Return length
	return sqrt(lengthSquared);
}

void Vector::normalize() noexcept {

	// Get length
	float length = getLength();
	
	// Normalize components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		(*this)[i] /= length;
}

Vector Vector::operator+(const Vector &addend) const noexcept {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		vector[i] = (*this)[i] + addend[i];
	
	// Return vector
	return vector;
}

Vector &Vector::operator+=(const Vector &addend) noexcept {

	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		(*this)[i] += addend[i];
	
	// Return self
	return *this;
}

Vector Vector::operator-(const Vector &subtrahend) const noexcept {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		vector[i] = (*this)[i] - subtrahend[i];
	
	// Return vector
	return vector;
}

Vector &Vector::operator-=(const Vector &subtrahend) noexcept {

	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		(*this)[i] -= subtrahend[i];
	
	// Return self
	return *this;
}

Vector Vector::operator*(float multiplier) const noexcept {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		vector[i] = (*this)[i] * multiplier;
	
	// Return vector
	return vector;
}

Vector &Vector::operator*=(float multiplier) noexcept {

	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		(*this)[i] *= multiplier;
	
	// Return self
	return *this;
}

Vector Vector::operator/(float divisor) const noexcept {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		vector[i] = (*this)[i] / divisor;
	
	// Return vector
	return vector;
}

Vector &Vector::operator/=(float divisor) noexcept {

	// Set vector components
	for(int8_t i = NUMBER_OF_COMPONENTS - 1; i >= 0; --i)
		(*this)[i] /= divisor;
	
	// Return self
	return *this;
}

const float& Vector::operator[](int index) const noexcept {
	
	// Return indexed value
	switch(index) {
	
		case 0:
			return x;
		
		case 1:
			return y;
		
		case 2:
			return z;
		
		case 3:
		default:
			return e;
	}
}

float &Vector::operator[](int index) noexcept {
	
	// Return indexed value
	switch(index) {
	
		case 0:
			return x;

		case 1:
			return y;
		
		case 2:
			return z;
		
		case 3:
		default:
			return e;
	}
}

Vector &Vector::operator=(const Vector &vector) noexcept {

	// Copy values and return self
	return *reinterpret_cast<Vector *>(memmove(this, &vector, sizeof(Vector)));
}
