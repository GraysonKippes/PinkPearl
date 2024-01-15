#include "extent.h"

uint64_t extent_area(const extent_t extent) {
	return (uint64_t)extent.width * (uint64_t)extent.length;
}

offset_t offset_add(const offset_t a, const offset_t b) {
	return (offset_t){
		.x = a.x + b.x,
		.y = a.y + b.y
	};
}

offset_t offset_subtract(const offset_t a, const offset_t b) {
	return (offset_t){
		.x = a.x - b.x,
		.y = a.y - b.y
	};
}
