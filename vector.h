// Header gaurd
#ifndef VECTOR_H
#define VECTOR_H


// Vector class
class Vector {

	// Public
	public:
	
		/*
		Name: Constructor
		Purpose: Allows setting vector components upon creation
		*/
		Vector(float x = 0, float y = 0, float z = 0, float e = 0);
		
		/*
		Name: Copy Constructor
		Purpose: Initializes a copy of a vector
		*/
		Vector(Vector &value);
		
		/*
		Name: Get Length
		Purpose: Returns length of vector
		*/
		float getLength() const;
		
		/*
		Name: Normalize
		Purpose: Normalizes vector components
		*/
		void normalize();
		
		/*
		Name: Addition operator
		Purpose: Allows adding vectors
		*/
		Vector operator+(const Vector &addend) const;
		Vector &operator+=(const Vector &addend);
		
		/*
		Name: Subtraction operator
		Purpose: Allows subtracting vectors
		*/
		Vector operator-(const Vector &subtrahend) const;
		Vector &operator-=(const Vector &addend);
		
		/*
		Name: Multiplication operator
		Purpose: Allows scaling a vector
		*/
		Vector operator*(float multiplier) const;
		Vector &operator*=(float multiplier);
		
		/*
		Name: Division operator
		Purpose: Allows shrinking a vector
		*/
		Vector operator/(float divisor) const;
		Vector &operator/=(float divisor);
		
		/*
		Name: Subscript operator
		Purpose: Allows addressing vector components
		*/
		const float& operator[](int index) const;
		float& operator[](int index);
		
		/*
		Name: Assignment operator
		Purpose: Allows copying a vector
		*/
		Vector &operator=(const Vector &vector);
	
		// Vector components
		float x;
		float y;
		float z;
		float e;
};


#endif
