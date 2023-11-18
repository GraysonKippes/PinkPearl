#include "animation.h"

#include <stddef.h>

bool is_animation_set_empty(animation_set_t animation_set) {
	return animation_set.num_animations == 0 || animation_set.animations == NULL;
}
