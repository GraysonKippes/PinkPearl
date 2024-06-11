#ifndef LERP_H
#define LERP_H

// Returns a floating-point value which is between 0 and 1 when x is between start and end.
#define NORMALIZE(start, end, x) ((double)(x - start) / (double)(end - start))

// Linear interpolation with single-precision floating-point numbers.
float interpolateLinear(const float start, const float end, const float delta);

#endif	// LERP_H