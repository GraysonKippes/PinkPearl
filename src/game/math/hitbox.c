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
	return x > a && x < b;
}

// Returns true if there is overlap on a single axis; returns false otherwise.
static bool rect_overlap_single_axis(const double a1, const double a2, const double b1, const double b2) {
	return is_number_between(a1, b1, b2) || is_number_between(a2, b1, b2) || (a1 <= b1 && a2 >= b2);
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

vector3D_cubic_t resolve_collision(const entity_transform_t entity_transform, const rect_t entity_hitbox, const rect_t collision_box) {
	
	// Collision detection and correction works by transforming 
	// the entity's velocity vector into a linear function:
	// 
	// y = mx + b, { s < x < e }, where
	//
	// x is the independent variable and represents the entity's position in the x-axis,
	// y is the dependent variable and represents the entity's position in the y-axis,
	// m is the slope of the function and the ratio of y component of the velocity to the x component,
	// b is the y-intercept of the function,
	// s is the lower bound of the domain of the function and represents 
	// 	the starting point of the entity's travel along the x-axis, and
	// e is the upper bound of the domain of the function and represents
	// 	the ending point of the entity's travel along the x-axis.
	//
	// Four such functions can be generated, using x as the independent variable 
	// and y as the dependent variable. Four more can be generated swapping the axes,
	// which effectively checks for collision in the y-direction instead of the x-direction.
	// One function is generated for each corner of the entity's hitbox, for these are the
	// extremities of the entity's body. With the extremities, collision can be checked
	// with the following independently sufficient conditions:
	//
	// 1. Any extremity is both above the lower bound and below the upper bound of the wall, OR
	// 2. The lower extremity is above the lower bound of the wall, and the upper extremity
	// 	is below the upper bound of the wall.
	//
	// Any of these conditions being fulfilled means that collision has happened.
	// 
	// There are two extremities when checking collision on a particular axis. 
	// Though there are four corners, only two are "in front of" the entity,
	// and can collide with a wall that the entity may move into.
	// Because of this, two linear functions are generated, one for each of those
	// two corners--one upper function and one lower function.
	// In order to generate these functions, first the base linear function
	// must be transposed such that the origin is at the old position of the entity:
	//
	// y - old_position.y = m(x - old_position.x).
	//
	// This is also known as 'point-slope form' of a linear function.
	// The function then can be rearranged back into 'slope-intercept form':
	//
	// y = m(x - old_position.x) + old_position.y.
	//
	// Keep in mind that the domain of the function is also transposed:
	//
	// y - old_position.y = m(x - old_position.x) { s < x - old_position.x < e }; 
	// y = m(x - old_position.x) + old_position.y { s + old_position.x < x < e + old_position.x }.
	// 
	// The function now maps to the travel of the entity when graphed:
	//
	// +y |
	//    |
	//  6 |      B
	//    |     /
	//  4 |    /
	//    |   /
	//  2 |  A
	//    |
	//    O---------------------------
	//    	2 4 6 8 |    |    |    |+x
	//    		10   15   20   25
	//
	// The capital A represents the old position of the entity, and
	// the capital B represents the projected new position of the entity.
	//
	// This process can be repeated for each of the two extremities to produce
	// the lower and upper functions. When moving right, which is the +x direction,
	// the +x side of the hitbox will be the outward extremity, and is represented with hitbox.x2.
	// When moving left, which is the -x direction, the -x side of 
	// the hitbox will be the outward extremity, and is represented with hitbox.x1.
	// The lower extremity will always use the lower side of the hitbox, which is
	// represented with hitbox.y1. The upper extremity will always use the upper side
	// of the hitbox, which is represented with hitbox.y2.
	//
	// Assuming moving right for convenience, the lower function is created like this:
	//
	// y - hitbox.y1 = m((x - hitbox.x2) - old_position.x) + old_position.y 
	// 	{ s + old_position.x < x - hitbox.x2 < e + old_position.x };
	// y = m(x - hitbox.x2 - old_position.x) + old_position.y + hitbox.y1 
	// 	{ s + old_position.x + hitbox.x2 < x < e + old_position.x + hitbox.x2 }.
	//
	// Similarly, the upper function is created like this:
	//
	// y - hitbox.y2 = m((x - hitbox.x2) - old_position.x) + old_position.y 
	// 	{ s + old_position.x < x - hitbox.x2 < e + old_position.x };
	// y = m(x - hitbox.x2 - old_position.x) + old_position.y + hitbox.y2 
	// 	{ s + old_position.x + hitbox.x2 < x < e + old_position.x + hitbox.x2 }.
	//
	// When x is set to an x-position, then y equals the y-position along the line of travel.
	// Theoretically, this extends out to infinity, but we are only concerned with the entity's
	// movement in a single step, which is not infinite; thus, we restrict the function to a domain
	// representing the start and end point of the entity's travel.
	// Now it is possible to check for collision, by finding the exact y-position of each of 
	// the extremities in the entity's travel, and then comparing these y-positions to the
	// upper and lower y-positions of the wall, using the above conditions:
	//
	// if wall.y1 < y_lower < wall.y2 OR wall.y1 < y_upper < wall.y2 
	// 	OR (y_lower < wall.y1 AND y_upper > wall.y2), then
	//
	// 	the entity collides.
	//
	//

	static const double friction_factor = 0.75;

	const vector3D_cubic_t old_position = entity_transform.position;
	const vector3D_cubic_t position_step = vector3D_spherical_to_cubic(entity_transform.velocity);
	const vector3D_cubic_t new_position = vector3D_cubic_add(old_position, position_step);

	vector3D_cubic_t resolved_position = new_position; 

	if (position_step.x > 0.0) {

		// If the wall is entirely behind or ahead of the entity as it moves right, then skip collision resolution.
		if (old_position.x + entity_hitbox.x1 >= collision_box.x2
				|| new_position.x + entity_hitbox.x2 <= collision_box.x1) {
			goto skip;
		}

		const double slope_y = position_step.y / position_step.x;
		const double x_boundary = collision_box.x1 - old_position.x - entity_hitbox.x2;
		const double lower_intersection = slope_y * x_boundary + old_position.y + entity_hitbox.y1;
		const double upper_intersection = slope_y * x_boundary + old_position.y + entity_hitbox.y2;
		
		const bool lower_collision = lower_intersection > collision_box.y1 && lower_intersection < collision_box.y2;
		const bool upper_collision = upper_intersection > collision_box.y1 && upper_intersection < collision_box.y2;
		const bool surround_collision = lower_intersection < collision_box.y1 && upper_intersection > collision_box.y2;

		// Test for lower and upper collision in the -x direction.
		if (lower_collision || upper_collision || surround_collision) {

			// If the entity is flush against the wall, then strip the x-component from the entity's velocity.
			if (old_position.x + entity_hitbox.x2 == collision_box.x1) {
				vector3D_cubic_t resolved_step = position_step;
				resolved_step.x = 0.0;
				resolved_step.y *= friction_factor;
				resolved_position = vector3D_cubic_add(old_position, resolved_step);
			}
			else {
				resolved_position = new_position;
				resolved_position.x = collision_box.x1 - entity_hitbox.x2;
				resolved_position.y = slope_y * (resolved_position.x - old_position.x) + old_position.y;
			}
			goto correct;
		}
	}
	else if (position_step.x < 0.0) {
		
		// If the wall is entirely behind or ahead of the entity as it moves left, then skip collision resolution.
		if (old_position.x + entity_hitbox.x2 <= collision_box.x1
				|| new_position.x + entity_hitbox.x1 >= collision_box.x2) {
			goto skip;
		}


		const double slope_y = position_step.y / position_step.x;
		const double x_boundary = collision_box.x2 - old_position.x - entity_hitbox.x1;
		const double lower_intersection = slope_y * x_boundary + old_position.y + entity_hitbox.y1;
		const double upper_intersection = slope_y * x_boundary + old_position.y + entity_hitbox.y2;
		
		const bool lower_collision = lower_intersection > collision_box.y1 && lower_intersection < collision_box.y2;
		const bool upper_collision = upper_intersection > collision_box.y1 && upper_intersection < collision_box.y2;
		const bool surround_collision = lower_intersection < collision_box.y1 && upper_intersection > collision_box.y2;

		// Test for lower and upper collision in the -x direction.
		if (lower_collision || upper_collision || surround_collision) {

			// If the entity is flush against the wall, then strip the x-component from the entity's velocity.
			if (old_position.x + entity_hitbox.x1 == collision_box.x2) {
				vector3D_cubic_t resolved_step = position_step;
				resolved_step.x = 0.0;
				resolved_step.y *= friction_factor;
				resolved_position = vector3D_cubic_add(old_position, resolved_step);
			}
			else {
				resolved_position = new_position;
				resolved_position.x = collision_box.x2 - entity_hitbox.x1;
				resolved_position.y = slope_y * (resolved_position.x - old_position.x) + old_position.y;
			}
			goto correct;
		}
	}

skip:
	return new_position;

correct:
	return resolved_position;
}
