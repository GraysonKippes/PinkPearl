#ifndef PROJECTION_H
#define PROJECTION_H

typedef struct projection_bounds_t {
	float m_left;
	float m_right;
	float m_bottom;
	float m_top;
	float m_near;
	float m_far;
} projection_bounds_t;

#endif	// PROJECTION_H
