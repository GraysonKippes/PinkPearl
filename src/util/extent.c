#include "extent.h"

uint64_t extent_area(const extent_t extent) {
	return (uint64_t)extent.width * (uint64_t)extent.length;
}
