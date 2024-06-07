#include "extent.h"

bool extent_equals(const Extent a, const Extent b) {
	return a.width == b.width && a.length == b.length;
}

Extent extent_scale(const Extent extent, const uint32_t factor) {
	return (Extent){
		.width = extent.width * factor,
		.length = extent.length * factor
	};
}

uint64_t extentArea(const Extent extent) {
	return (uint64_t)extent.width * (uint64_t)extent.length;
}