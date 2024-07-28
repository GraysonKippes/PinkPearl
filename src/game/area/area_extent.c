#include "area_extent.h"

#include "log/logging.h"

area_extent_t new_area_extent(const int x1, const int y1, const int x2, const int y2) {

	if (x1 >= x2) {
		logMsgF(LOG_LEVEL_WARNING, "Warning creating new area extent: x1 (%i) is not less than x2 (%i).", x1, x2);
		return new_area_extent(x2, y1, x1, y2);
	}

	if (y1 >= y2) {
		logMsgF(LOG_LEVEL_WARNING, "Warning creating new area extent: y1 (%i) is not less than y2 (%i).", y1, y2);
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

int area_extent_index(const area_extent_t area_extent, const Offset position) {
	
	if (position.x < area_extent.x1 || position.x > area_extent.x2 || position.y < area_extent.y1 || position.y > area_extent.y2) {
		logMsgF(LOG_LEVEL_ERROR, "Error calculating area extent index: position (%i, %i) does not fall within area extent [(%i, %i), (%i, %i)].",
			position.x, position.y, area_extent.x1, area_extent.y1, area_extent.x2, area_extent.y2);
		return -1;
	}

	const int offset_x = position.x - area_extent.x1;
	const int offset_y = position.y - area_extent.y1;

	return (offset_y * area_extent_width(area_extent)) + offset_x;
}
