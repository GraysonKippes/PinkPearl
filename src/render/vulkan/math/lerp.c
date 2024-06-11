#include "lerp.h"

float interpolateLinear(const float start, const float end, const float delta) {
	return start + delta * (end - start);
}