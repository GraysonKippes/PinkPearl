#ifndef AREA_EXTENT_H
#define AREA_EXTENT_H

#include "math/offset.h"

// TODO - replace with DimensionsI
typedef struct area_extent_t {
	int x1;
	int y1;
	int x2;
	int y2;
} area_extent_t;

area_extent_t new_area_extent(const int x1, const int y1, const int x2, const int y2);

int area_extent_width(const area_extent_t area_extent);
int area_extent_length(const area_extent_t area_extent);

// Returns a one-dimension index into the `area_extent` that corresponds to `position`.
// Because room position in an area is in the category of game-space coordinates, 
// 	they use the bottom-left to top-right orientation; in other words,
// 	the smallest coordinates represent the bottom-left corner of the area, and
// 	the largest coordinates represent the top-right corner of the area.
// If `position` does not correspond to any position in `area_extent`,
// 	then this function returns a negative value.
int area_extent_index(const area_extent_t area_extent, const offset_t position);

#endif	// AREA_EXTENT_H
