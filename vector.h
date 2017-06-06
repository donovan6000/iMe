// Header guard
#ifndef VECTOR_H
#define VECTOR_H


// Vector class
class Vector {

	// Public
	public:
	
		// Initialize
		void initialize(float x = 0, float y = 0, float z = 0, float e = 0);
		
		// Get Length
		float getLength() const;
		
		// Normalize
		void normalize();
		
		// Addition operator
		Vector operator+(const Vector &addend) const;
		Vector &operator+=(const Vector &addend);
		
		// Subtraction operator
		Vector operator-(const Vector &subtrahend) const;
		Vector &operator-=(const Vector &subtrahend);
		
		// Multiplication operator
		Vector operator*(float multiplier) const;
		Vector &operator*=(float multiplier);
		
		// Division operator
		Vector operator/(float divisor) const;
		Vector &operator/=(float divisor);
		
		// Subscript operator
		const float& operator[](int index) const;
		float& operator[](int index);
		
		// Assignment operator
		Vector &operator=(const Vector &vector);
	
		// Vector components
		float x;
		float y;
		float z;
		float e;
};


#endif
