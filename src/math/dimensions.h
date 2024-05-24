#ifndef DIMENSIONS_H
#define DIMENSIONS_H

// TODO - use in lieu of area_extent_t
typedef struct DimensionsI {
	int x1, y1;
	int x2, y2;
} DimensionsI;

typedef struct DimensionsF {
	float x1, y1;
	float x2, y2;
} DimensionsF;

#endif	// DIMENSIONS_H