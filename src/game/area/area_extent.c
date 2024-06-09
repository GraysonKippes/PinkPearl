#include "area_extent.h"

#include "log/logging.h"

area_extent_t new_area_extent(const int x1, const int y1, const int x2, const int y2) {

	if (x1 >= x2) {
		logMsgF(WARNING, "Warning creating new area extent: x1 (%i) is not less than x2 (%i).", x1, x2);
		return new_area_extent(x2, y1, x1, y2);
	}

	if (y1 >= y2) {
		logMsgF(WARNING, "Warning creating new area extent: y1 (%i) is not less than y2 (%i).", y1, y2);
		return new_area_extent(x1, y2, x2, y1);
	}

	return (area_extent_t){
		.x1 = x1,
		.y1 = y1,
		.x2 = x2,
		.y2 = y2
	};
}

int area_extent_width(const area_extent_t area_extent) {
	return area_extent.x2 - area_extent.x1 + 1;
}

int area_extent_length(const area_extent_t area_extent) {
	return area_extent.y2 - area_extent.y1 + 1;
}

int area_extent_index(const area_extent_t area_extent, const offset_t position) {
	
	if (position.x < area_extent.x1) {
		return -1;
	}

	if (position.x > area_extent.x2) {
		return -2;
	}

	if (position.y < area_extent.y1) {
		return -3;
	}

	if (position.y > area_extent.y2) {
		return -4;
	}

	const int offset_x = position.x - area_extent.x1;
	const int offset_y = position.y - area_extent.y1;

	return (offset_y * area_extent_width(area_extent)) + offset_x;
}
