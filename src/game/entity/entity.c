#include "entity.h"

#include <stddef.h>

#include "game/game.h"
#include "render/render_object.h"
#include "render/vulkan/math/render_vector.h"

#define SQUARE(x) ((x) * (x))

static Vector3D resolve_collision(const Vector3D old_position, const Vector3D new_position, const BoxD hitbox, const BoxD wall);

entity_t new_entity(void) {
	return (entity_t){
		.transform = (entity_transform_t){ },
		.hitbox = (BoxD){ },
		.ai = entityAINull,
		.render_handle = renderHandleInvalid
	};
}

void tick_entity(entity_t *pEntity) {
	if (!pEntity) {
		return;
	}

	const Vector3D old_position = pEntity->transform.position;
	const Vector3D position_step = pEntity->transform.velocity;
	Vector3D new_position = vector3D_add(old_position, position_step);

	// The square of the distance of the currently selected new position from the old position.
	// This variable is used to track which resolved new position is the shortest from the entity.
	// The squared length is used instead of the real length because it is only used for comparison.
	double step_length_squared = SQUARE(position_step.x) + SQUARE(position_step.y) + SQUARE(position_step.z); 

	for (unsigned int i = 0; i < currentArea.pRooms[currentArea.currentRoomIndex].wallCount; ++i) {
		
		const BoxD wall = currentArea.pRooms[currentArea.currentRoomIndex].pWalls[i];

		Vector3D resolved_position = resolve_collision(old_position, new_position, pEntity->hitbox, wall);
		Vector3D resolved_step = vector3D_subtract(resolved_position, old_position);
		const double resolved_step_length_squared = SQUARE(resolved_step.x) + SQUARE(resolved_step.y) + SQUARE(resolved_step.z);

		if (resolved_step_length_squared < step_length_squared) {
			new_position = resolved_position;
			step_length_squared = resolved_step_length_squared;
		}
	}

	pEntity->transform.position = new_position;

	// Update render object.
	const int renderHandle = pEntity->render_handle;
	renderObjectSetPosition(renderHandle, 0, pEntity->transform.position);
}

/* -- Copied from hitbox.h -- */

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
 * o = cd + ef + gh + cdef			Original boolean function
 * o = cd + cdef + ef + gh			Commutation
 * o = (cd + cdef) + ef + gh		Association
 * o = ((cd) + (cd)ef) + ef + gh	Association
 * o = (cd) + ef + gh				Absorption
 * o = cd + ef + gh					Association
 *
 * cd -> "a.x1 is between b.x1 and b.x2"
 * ef -> "a.x2 is between b.x1 and b.x2"
 *
 * */

static Vector3D resolve_collision(const Vector3D old_position, const Vector3D new_position, const BoxD hitbox, const BoxD wall) {
	
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
	// Four such functions can be generated, one for each corner of the hitbox, 
	// using x as the independent variable and y as the dependent variable. 
	// Four more can be generated swapping the axes,
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
	//    	2 4 6 8 |    |    |    | +x
	//    		   10   15   20   25
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

	// This parameter determines how much the entity is slowed down when sliding against a wall.
	static const double friction_factor = 0.75;

	const Vector3D position_step = vector3D_subtract(new_position, old_position);

	Vector3D resolved_position = new_position; 

	if (position_step.x > 0.0) {

		// If the wall is entirely behind or ahead of the entity as it moves right, then skip collision resolution.
		if (old_position.x + hitbox.x1 >= wall.x2 || new_position.x + hitbox.x2 <= wall.x1) {
			goto skip;
		}

		const double slope_y = position_step.y / position_step.x;
		const double x_boundary = wall.x1 - old_position.x - hitbox.x2;
		const double lower_intersection = slope_y * x_boundary + old_position.y + hitbox.y1;
		const double upper_intersection = slope_y * x_boundary + old_position.y + hitbox.y2;
		
		const bool lower_collision = lower_intersection > wall.y1 && lower_intersection < wall.y2;
		const bool upper_collision = upper_intersection > wall.y1 && upper_intersection < wall.y2;
		const bool surround_collision = lower_intersection < wall.y1 && upper_intersection > wall.y2;

		// Test for lower and upper collision in the +x direction.
		if (lower_collision || upper_collision || surround_collision) {

			// If the entity is flush against the wall, then strip the x-component from the entity's velocity.
			if (old_position.x + hitbox.x2 == wall.x1) {
				Vector3D resolved_step = position_step;
				resolved_step.x = 0.0;
				resolved_step.y *= friction_factor;
				resolved_position = vector3D_add(old_position, resolved_step);
			}
			else {
				resolved_position = new_position;
				resolved_position.x = wall.x1 - hitbox.x2;
				resolved_position.y = slope_y * (resolved_position.x - old_position.x) + old_position.y;
			}
			goto correct;
		}
	}
	else if (position_step.x < 0.0) {
		
		// If the wall is entirely behind or ahead of the entity as it moves left, then skip collision resolution.
		if (old_position.x + hitbox.x2 <= wall.x1 || new_position.x + hitbox.x1 >= wall.x2) {
			goto skip;
		}

		const double slope_y = position_step.y / position_step.x;
		const double x_boundary = wall.x2 - old_position.x - hitbox.x1;
		const double lower_intersection = slope_y * x_boundary + old_position.y + hitbox.y1;
		const double upper_intersection = slope_y * x_boundary + old_position.y + hitbox.y2;
		
		const bool lower_collision = lower_intersection > wall.y1 && lower_intersection < wall.y2;
		const bool upper_collision = upper_intersection > wall.y1 && upper_intersection < wall.y2;
		const bool surround_collision = lower_intersection < wall.y1 && upper_intersection > wall.y2;

		// Test for lower and upper collision in the -x direction.
		if (lower_collision || upper_collision || surround_collision) {

			// If the entity is flush against the wall, then strip the x-component from the entity's velocity.
			if (old_position.x + hitbox.x1 == wall.x2) {
				Vector3D resolved_step = position_step;
				resolved_step.x = 0.0;
				resolved_step.y *= friction_factor;
				resolved_position = vector3D_add(old_position, resolved_step);
			}
			else {
				resolved_position = new_position;
				resolved_position.x = wall.x2 - hitbox.x1;
				resolved_position.y = slope_y * (resolved_position.x - old_position.x) + old_position.y;
			}
			goto correct;
		}
	}

	if (position_step.y > 0.0) {
		
		// If the wall is entirely below or above the entity as it moves up, then skip collision resolution.
		if (old_position.y + hitbox.y1 >= wall.y2 || new_position.y + hitbox.y2 <= wall.y1) {
			goto skip;
		}

		const double slope_x = position_step.x / position_step.y;
		const double y_boundary = wall.y1 - old_position.y - hitbox.y2;
		const double left_intersection = slope_x * y_boundary + old_position.x + hitbox.x1;
		const double right_intersection = slope_x * y_boundary + old_position.x + hitbox.x2;
		
		const bool left_collision = left_intersection > wall.x1 && left_intersection < wall.x2;
		const bool right_collision = right_intersection > wall.x1 && right_intersection < wall.x2;
		const bool surround_collision = left_intersection < wall.x1 && right_intersection > wall.x2;

		// Test for left and right collision in the +y direction.
		if (left_collision || right_collision || surround_collision) {

			// If the entity is flush against the wall, then strip the y-component from the entity's velocity.
			if (old_position.y + hitbox.y2 == wall.y1) {
				Vector3D resolved_step = position_step;
				resolved_step.y = 0.0;
				resolved_step.x *= friction_factor;
				resolved_position = vector3D_add(old_position, resolved_step);
			}
			else {
				resolved_position = new_position;
				resolved_position.y = wall.y1 - hitbox.y2;
				resolved_position.x = slope_x * (resolved_position.y - old_position.y) + old_position.x;
			}
			goto correct;
		}
	}
	else if (position_step.y < 0.0) {

		// If the wall is entirely below or above the entity as it moves down, then skip collision resolution.
		if (old_position.y + hitbox.y2 <= wall.y1 || new_position.y + hitbox.y1 >= wall.y2) {
			goto skip;
		}

		const double slope_x = position_step.x / position_step.y;
		const double y_boundary = wall.y2 - old_position.y - hitbox.y1;
		const double left_intersection = slope_x * y_boundary + old_position.x + hitbox.x1;
		const double right_intersection = slope_x * y_boundary + old_position.x + hitbox.x2;
		
		const bool left_collision = left_intersection > wall.x1 && left_intersection < wall.x2;
		const bool right_collision = right_intersection > wall.x1 && right_intersection < wall.x2;
		const bool surround_collision = left_intersection < wall.x1 && right_intersection > wall.x2;

		// Test for left and right collision in the -y direction.
		if (left_collision || right_collision || surround_collision) {

			// If the entity is flush against the wall, then strip the y-component from the entity's velocity.
			if (old_position.y + hitbox.y1 == wall.y2) {
				Vector3D resolved_step = position_step;
				resolved_step.y = 0.0;
				resolved_step.x *= friction_factor;
				resolved_position = vector3D_add(old_position, resolved_step);
			}
			else {
				resolved_position = new_position;
				resolved_position.y = wall.y2 - hitbox.y1;
				resolved_position.x = slope_x * (resolved_position.y - old_position.y) + old_position.x;
			}
			goto correct;
		}
	}

skip:
	return new_position;

correct:
	return resolved_position;
}
