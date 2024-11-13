#ifndef ENTITY_PHYSICS_H
#define ENTITY_PHYSICS_H

#include "math/Vector.h"

typedef struct EntityPhysics {
	
	Vector3D position;
	
	Vector3D velocity;
	
	Vector3D acceleration;
	
} EntityPhysics;

#endif	// ENTITY_PHYSICS_H