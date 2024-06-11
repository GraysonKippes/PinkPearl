#include "offset.h"

Offset offset_add(const Offset a, const Offset b) {
	return (Offset){
		.x = a.x + b.x,
		.y = a.y + b.y
	};
}

Offset offset_subtract(const Offset a, const Offset b) {
	return (Offset){
		.x = a.x - b.x,
		.y = a.y - b.y
	};
}
