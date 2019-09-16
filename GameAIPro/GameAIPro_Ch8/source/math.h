#ifndef __MATH__
#define __MATH__

#include <cmath>

// min and max
template <typename T> T min(T a, T b) { return (b < a ? b : a); }
template <typename T> T max(T a, T b) { return (b > a ? b : a); }

// limit a value to between two values
template <typename T> T limit(T lowerBound, T value, T upperBound) {
	return max(lowerBound, min(value, upperBound));
}

// square a value
template <typename T> T SQ(T value) { return (value * value); }

// linearly interpolation between two values
template <typename T> T lerp(T vMin, T vMax, float pct) {
	return vMin + ((vMax - vMin) * pct);
}

// linearly interpolate a value between two ranges
template <typename T> T linterp(T srcMin, T srcMax, T dstMin, T dstMax, T value) {
	return dstMin + ((dstMax - dstMin) * ((value - srcMin) / (srcMax - srcMin)));
}

// constants
#define PI (3.14159265f)
#define TWOPI (2 * PI)

// trig
inline void sinCos(float &sinAngle, float &cosAngle, float angleInRadians) {
	cosAngle = cosf(angleInRadians);
	sinAngle = (1.0f - cosAngle);
}

#endif // __MATH__
