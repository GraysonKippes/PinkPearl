#include "hitbox.h"

#include <math.h>

#define SQUARE(x) (x * x)

rect_t hitbox_to_world_space(hitbox_t hitbox, vector3D_cubic_t position) {
	return (rect_t){
		.x1 = position.x - hitbox.width / 2.0,
		.y1 = position.y - hitbox.length / 2.0,
		.x2 = position.x + hitbox.width / 2.0,
		.y2 = position.y + hitbox.length / 2.0
	};
}

// Returns true if `x` is between `a` and `b`, inclusive: x -> [a, b].
static bool is_number_between(const double x, const double a, const double b) {
	return x >= a && x <= b;
}

// Returns true if there is overlap on a single axis; returns false otherwise.
static bool rect_overlap_single_axis(const double a1, const double a2, const double b1, const double b2) {
	return is_number_between(a1, b1, b2) || is_number_between(a2, b1, b2) || (a1 < b1 && a2 > b2);
}

bool rect_overlap(const rect_t a, const rect_t b) {
	return rect_overlap_single_axis(a.x1, a.x2, b.x1, b.x2) && rect_overlap_single_axis(a.y1, a.y2, b.y1, b.y2);
}

/* SINGLE-AXIS OVERLAP CASES
 *
 * Case 1: Rect A ahead of rect B
 * 	a.x1 > b.x1 AND
 * 	a.x1 < b.x2
 *
 * Case 2: Rect A behind rect B
 * 	a.x2 > b.x1 AND
 * 	a.x2 < b.x2
 *
 * Case 3: Rect A larger than & around rect B
 * 	a.x1 < b.x1 AND
 * 	a.x2 > b.x2
 *
 * Case 4: Rect A smaller than & within rect B
 * 	a.x1 > b.x1 AND
 * 	a.x1 < b.x2 AND
 * 	a.x2 > b.x1 AND
 * 	a.x2 < b.x2
 *
 * let bool c = a.x1 > b.x1
 * let bool d = a.x1 < b.x2
 * let bool e = a.x2 > b.x1
 * let bool f = a.x2 < b.x2
 * let bool g = a.x1 < b.x1
 * let bool h = a.x2 > b.x2
 *
 * case 1 = cd
 * case 2 = ef
 * case 3 = gh
 * case 4 = cdef
 * 
 * let bool o = true if overlap, false if no overlap
 * o = case 1 OR case 2 OR case 3 OR case 4
 * o = cd + ef + gh + cdef		Original boolean function
 * o = cd + cdef + ef + gh		Commutation
 * o = (cd + cdef) + ef + gh		Association
 * o = ((cd) + (cd)ef) + ef + gh	Association
 * o = (cd) + ef + gh			Absorption
 * o = cd + ef + gh			Association
 *
 * cd -> "a.x1 is between b.x1 and b.x2"
 * ef -> "a.x2 is between b.x1 and b.x2"
 *
 * */

#include "log/logging.h"

double resolve_collision(const entity_transform_t entity_transform, const rect_t entity_hitbox, const rect_t collision_box) {
	
	const vector3D_cubic_t old_position = entity_transform.position;
	const vector3D_cubic_t position_step = vector3D_spherical_to_cubic(entity_transform.velocity);
	const vector3D_cubic_t new_position = vector3D_cubic_add(old_position, position_step);

	vector3D_cubic_t resolved_position = new_position; 

	if (position_step.x > 0.0) {

		// Moving right (+x direction)

		// The padding that the entity hitbox gives; depends on the direction of movement.
		const double x_boundary = entity_hitbox.x2;

		// Is the entity behind the collision box before moving?
		const bool precondition = old_position.x - x_boundary <= collision_box.x2;

		// Is the entity either inside or ahead of the collision box after moving?
		const bool postcondition = new_position.x + x_boundary >= collision_box.x1;

		if (precondition && postcondition) {
			resolved_position.x = collision_box.x1 - x_boundary;
			//logf_message(INFO, "Colliding going right: %.2f -> %.2f", new_position.x, resolved_position.x);
		}
		else {
			//return;
		}
	}
	else if (position_step.x < 0.0) {

		// Moving left (-x direction)

		// The padding that the entity hitbox gives; depends on the direction of movement.
		const double x_boundary = entity_hitbox.x1;

		// Is the entity behind the collision box before moving?
		const bool precondition = old_position.x + x_boundary >= collision_box.x1;

		// Is the entity either inside or ahead of the collision box after moving?
		const bool postcondition = new_position.x - x_boundary <= collision_box.x2;

		if (precondition && postcondition) {
			resolved_position.x = collision_box.x2 + x_boundary;
			//logf_message(INFO, "Colliding going left: %.2f -> %.2f", new_position.x, resolved_position.x);
		}
		else {
			//return;
		}
	}

	const vector3D_cubic_t resolved_step = vector3D_cubic_subtract(resolved_position, old_position);
	const double step_length = entity_transform.velocity.r;
	const double resolved_step_length = sqrt(SQUARE(resolved_step.x) + SQUARE(resolved_step.y) + SQUARE(resolved_step.z));

	return fmin(resolved_step_length, step_length);
}

/* SINGLE-AXIS COLLISION RESOLUTION
 *
 * COLLISION CASES
 *
 * Case 1: colliding going right (+x)
 * 	old_pos.x - (hitbox.width / 2) <= rect.x2 AND
 * 	new_pos.x + (hitbox.width / 2) >= rect.x1
 *
 * Case 2: colliding going left (-x)
 * 	old_pos.x + (hitbox.width / 2) >= rect.x1 AND
 * 	new_pos.x - (hitbox.width / 2) <= rect.x2
 *
 * RESOLUTION CASES
 *
 * Case 0: no collision
 * 	new_pos = old_pos + step
 *
 * Case 1: colliding going right (+x)
 * 	new_pos.x = col_box.x1 - (hitbox.width / 2)
 *
 * Case 2: colliding going left (-x)
 * 	new_pos.x = col_box.x2 + (hitbox.width / 2)
 *
 * Case 3: no movement (step = 0)
 * 	new_pos = old_pos (same as case 0)
 *
 * */
