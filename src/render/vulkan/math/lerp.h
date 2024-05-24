#ifndef LERP_H
#define LERP_H

#define NORMALIZE_DELTA(start, end, x) ((double)(x - start) / (double)(end - start))

// Returns a double-precision floating-point value which is between 0 and 1 when val is between start and end.
double normalize_delta(const int start, const int end, const int x);

// Linear interpolation with single-precision floating-point numbers.
float lerpF(const float start, const float end, const float delta);

#endif	// LERP_H