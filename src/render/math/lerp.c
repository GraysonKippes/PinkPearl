#include "lerp.h"

double normalize_delta(const int start, const int end, const int x) {
	return (double)(x - start) / (double)(end - start);
}

float lerpF(const float start, const float end, const double delta) {
	return start + (float)delta * (end - start);
}