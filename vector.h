// Header guard
#ifndef VECTOR_H
#define VECTOR_H


// Vector class
class Vector final {

	// Public
	public:
	
		// Initialize
		void initialize(float x = 0, float y = 0, float z = 0, float e = 0) noexcept;
		
		// Get Length
		float getLength() const noexcept;
		
		// Normalize
		void normalize() noexcept;
		
		// Addition operator
		Vector operator+(const Vector &addend) const noexcept;
		Vector &operator+=(const Vector &addend) noexcept;
		
		// Subtraction operator
		Vector operator-(const Vector &subtrahend) const noexcept;
		Vector &operator-=(const Vector &subtrahend) noexcept;
		
		// Multiplication operator
		Vector operator*(float multiplier) const noexcept;
		Vector &operator*=(float multiplier) noexcept;
		
		// Division operator
		Vector operator/(float divisor) const noexcept;
		Vector &operator/=(float divisor) noexcept;
		
		// Subscript operator
		const float& operator[](int index) const noexcept;
		float& operator[](int index) noexcept;
		
		// Assignment operator
		Vector &operator=(const Vector &vector) noexcept;
	
		// Vector components
		float x;
		float y;
		float z;
		float e;
};


#endif
