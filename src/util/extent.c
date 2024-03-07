#include "extent.h"

bool extent_equals(const extent_t a, const extent_t b) {
	return a.width == b.width && a.length == b.length;
}

extent_t extent_scale(const extent_t extent, const uint32_t factor) {
	return (extent_t){
		.width = extent.width * factor,
		.length = extent.length * factor
	};
}

uint64_t extent_area(const extent_t extent) {
	return (uint64_t)extent.width * (uint64_t)extent.length;
}