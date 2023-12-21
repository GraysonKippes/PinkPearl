#include "hitbox.h"

hitbox_t hitbox_to_world_space(hitbox_t hitbox, vector3D_cubic_t position) {
	return (hitbox_t){
		.width = hitbox.width + position.x,
		.length = hitbox.length + position.y
	};
}
