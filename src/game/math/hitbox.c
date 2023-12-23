#include "hitbox.h"

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
