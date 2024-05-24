#include "offset.h"

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
