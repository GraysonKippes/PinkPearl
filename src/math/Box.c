#include "Box.h"

int boxWidthI(const BoxI box) {
	return box.x2 - box.x1;
}

int boxLengthI(const BoxI box) {
	return box.y2 - box.y1;
}

int boxAreaI(const BoxI box) {
	return (box.x2 - box.x1) * (box.y2 - box.y1);
}

float boxWidthF(const BoxF box) {
	return box.x2 - box.x1;
}

float boxLengthF(const BoxF box) {
	return box.y2 - box.y1;
}

float boxAreaF(const BoxF box) {
	return (box.x2 - box.x1) * (box.y2 - box.y1);
}

double boxWidthD(const BoxD box) {
	return box.x2 - box.x1;
}

double boxLengthD(const BoxD box) {
	return box.y2 - box.y1;
}

double boxAreaD(const BoxD box) {
	return (box.x2 - box.x1) * (box.y2 - box.y1);
}