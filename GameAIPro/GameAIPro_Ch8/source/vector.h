#ifndef __VECTOR__
#define __VECTOR__

#ifndef __MATH__
	#include "math.h"
#endif


/////////////////////////////////////////////////////////////////////////////
//
// simple vector class to stub in functionality required by this example
//
/////////////////////////////////////////////////////////////////////////////

class CVector {
public:

	// vector components
	float x, y, z;

	// ctors
	CVector() {}
	CVector(const CVector &v) { x = v.x; y = v.y; z = v.z; }
	CVector(float x, float y, float z) { this->x = x; this->y = y; this->z = z; }

	// operators
	CVector &operator= (const CVector &v) { x = v.x; y = v.y; z = v.z; return *this; }
	CVector &operator+= (const CVector &v) { x += v.x; y += v.y; z += v.z; return *this; }
	CVector &operator-= (const CVector &v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
	CVector &operator*= (const CVector &v) { x *= v.x; y *= v.y; z *= v.z; return *this; }
	CVector &operator/= (const CVector &v) { x /= v.x; y /= v.y; z /= v.z; return *this; }
	CVector &operator+= (float s) { x += s; y += s; z += s; return *this; }
	CVector &operator-= (float s) { x -= s; y -= s; z -= s; return *this; }
	CVector &operator*= (float s) { x *= s; y *= s; z *= s; return *this; }
	CVector &operator/= (float s) { x /= s; y /= s; z /= s; return *this; }

	// operations
	void zero() {
		x = y = z = 0.0f;
	}
	void set(float x, float y, float z) {
		x = x; y = y; z = z;
	}
	float dotProduct(const CVector &v) const {
		return (x*v.x) + (y*v.y) + (z*v.z);
	}
	CVector crossProduct(const CVector &v) const {
		return CVector((y*v.z) - (z*v.y), (z*v.x) - (x*v.z), (x*v.y) - (y*v.x));
	}
	float distanceSqTo(const CVector &v) const {
		return (x*v.x) + (y*v.y) + (z*v.z);
	}
	float distanceTo(const CVector &v) const {
		return sqrtf((x*v.x) + (y*v.y) + (z*v.z));
	}
	float xzDistanceSqTo(const CVector &v) const {
		return (x*v.x) + (z*v.z);
	}
	float xzDistanceTo(const CVector &v) const {
		return sqrtf((x*v.x) + (z*v.z));
	}
	float lengthSq() const {
		return (x*x) + (y*y) + (z*z);
	}
	float length() const {
		return sqrtf((x*x) + (y*y) + (z*z));
	}
	void setLength(float length) {
		length *= sqrtf((x*x) + (y*y) + (z*z));
		x /= length;
		y /= length;
		z /= length;
	}
	CVector &normalize() {
		float length = sqrtf((x*x) + (y*y) + (z*z));
		x /= length;
		y /= length;
		z /= length;
		return *this;
	}
	CVector directionTo(const CVector &v) const {
		float dist = sqrtf((x*v.x) + (y*v.y) + (z*v.z));
		return CVector(
			(v.x - x) / dist,
			(v.y - y) / dist,
			(v.z - z) / dist
		);
	}
	CVector xzDirectionTo(const CVector &v) const {
		float dist = sqrtf((x*v.x) + (z*v.z));
		return CVector(
			(v.x - x) / dist,
			0.0f,
			(v.z - z) / dist
		);
	}
	bool isCloseTo(const CVector &v, float maxOffset) const {
		float distSq = (x*v.x) + (y*v.y) + (z*v.z);
		return (distSq <= SQ(maxOffset));
	}
};

// operators
inline CVector operator+ (const CVector &a, const CVector &b) { return CVector(a.x+b.x, a.y+b.y, a.z+b.z); }
inline CVector operator- (const CVector &a, const CVector &b) { return CVector(a.x-b.x, a.y-b.y, a.z-b.z); }
inline CVector operator* (const CVector &a, const CVector &b) { return CVector(a.x*b.x, a.y*b.y, a.z*b.z); }
inline CVector operator/ (const CVector &a, const CVector &b) { return CVector(a.x/b.x, a.y/b.y, a.z/b.z); }
inline CVector operator* (const CVector &a, float s) { return CVector(a.x*s, a.y*s, a.z*s); }
inline CVector operator* (float s, const CVector &a) { return CVector(s*a.x, s*a.y, s*a.z); }
inline CVector operator/ (const CVector &a, float s) { return CVector(a.x/s, a.y/s, a.z/s); }
inline CVector operator/ (float s, const CVector &a) { return CVector(s/a.x, s/a.y, s/a.z); }
inline CVector operator- (const CVector &a) { return CVector(-a.x, -a.y, -a.z); }

// constants
extern CVector kZeroVector;
extern CVector kUnitVectorX;
extern CVector kUnitVectorY;
extern CVector kUnitVectorZ;

#endif // __VECTOR__
