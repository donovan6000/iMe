// Header files
#include <math.h>
#include <string.h>
#include "vector.h"


// Supporting function implementation
void Vector::initialize(float x, float y, float z, float e) {

	// Set vector components
	this->x = x;
	this->y = y;
	this->z = z;
	this->e = e;
}

float Vector::getLength() const {

	// Return length
	return sqrt(pow(x, 2) + pow(y, 2) + pow(z, 2) + pow(e, 2));
}

void Vector::normalize() {

	// Get length
	float length = getLength();
	
	// Normalize components
	x /= length;
	y /= length;
	z /= length;
	e /= length;
}

Vector Vector::operator+(const Vector &addend) const {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	vector.x = x + addend.x;
	vector.y = y + addend.y;
	vector.z = z + addend.z;
	vector.e = e + addend.e;
	
	// Return vector
	return vector;
}

Vector &Vector::operator+=(const Vector &addend) {

	// Set vector components
	x += addend.x;
	y += addend.y;
	z += addend.z;
	e += addend.e;
	
	// Return self
	return *this;
}

Vector Vector::operator-(const Vector &subtrahend) const {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	vector.x = x - subtrahend.x;
	vector.y = y - subtrahend.y;
	vector.z = z - subtrahend.z;
	vector.e = e - subtrahend.e;
	
	// Return vector
	return vector;
}

Vector &Vector::operator-=(const Vector &subtrahend) {

	// Set vector components
	x -= subtrahend.x;
	y -= subtrahend.y;
	z -= subtrahend.z;
	e -= subtrahend.e;
	
	// Return self
	return *this;
}

Vector Vector::operator*(float multiplier) const {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	vector.x = x * multiplier;
	vector.y = y * multiplier;
	vector.z = z * multiplier;
	vector.e = e * multiplier;
	
	// Return vector
	return vector;
}

Vector &Vector::operator*=(float multiplier) {

	// Set vector components
	x *= multiplier;
	y *= multiplier;
	z *= multiplier;
	e *= multiplier;
	
	// Return self
	return *this;
}

Vector Vector::operator/(float divisor) const {

	// Initialize variables
	Vector vector;
	
	// Set vector components
	vector.x = x / divisor;
	vector.y = y / divisor;
	vector.z = z / divisor;
	vector.e = e / divisor;
	
	// Return vector
	return vector;
}

Vector &Vector::operator/=(float divisor) {

	// Set vector components
	x /= divisor;
	y /= divisor;
	z /= divisor;
	e /= divisor;
	
	// Return self
	return *this;
}

const float& Vector::operator[](int index) const {
	
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

float &Vector::operator[](int index) {
	
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

Vector &Vector::operator=(const Vector &vector) {

	// Check if not calling on self
	if(this != &vector)

		// Copy values
		memcpy(this, &vector, sizeof(Vector));
	
	// Return self
	return *this;
}
