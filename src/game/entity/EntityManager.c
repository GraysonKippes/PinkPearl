#include "EntityManager.h"

#include <assert.h>
#include <stdlib.h>
#include "game/Game.h"
#include "log/Logger.h"
#include "render/RenderManager.h"
#include "util/Allocation.h"
#include "util/FileIO.h"
#include "util/Time.h"

#define ECS_ELEMENT_COUNT	64
#define MAX_ENTITY_COUNT 	(ECS_ELEMENT_COUNT - 1)

#define SQUARE(x) ((x) * (x))

// Requirements of entity component system:
//	* Systems must operate on all entities with *at least* all requirement components.
//	* Each component must reference back to the entity that it "owns" it.
//	* Each entity must have a bitmask indicating which components it has.
//	* Entity handle zero must be reserved to indicate a null entity.

// This struct includes only the intrinsic/immutable state in EntityPhysics.
typedef struct IntrEntityPhysics {
	double maxSpeed;
	double mass;
} IntrEntityPhysics;

// This struct includes only the intrinsic/immutable state in EntityHealth.
typedef struct IntrEntityHealth {
	int32_t maxHP;
} IntrEntityHealth;

// Contains the basic, immutable information of a species of entity, from which individuals of that species can be instantiated.
typedef struct EntityRecord2 {
	String entityID;
	uint32_t componentMask;
	EntityTraits traits;
	IntrEntityPhysics physics;
	BoxD hitbox;
	IntrEntityHealth health;
	EntityAI ai;
	String textureID;
	BoxF textureDimensions;
} EntityRecord2;

typedef struct EntityRegistry {
	size_t recordCount;
	EntityRecord2 *pRecords;
} EntityRegistry;

struct EntityComponentSystem_T {
	
	EntityRegistry registry;
	
	int32_t stackPosition; // Index of the top element in the stack.
	Entity2 entityStack[MAX_ENTITY_COUNT];
	
	bool slots[ECS_ELEMENT_COUNT];
	uint32_t componentMasks[ECS_ELEMENT_COUNT];
	
	EntityTraits 		traits[ECS_ELEMENT_COUNT];
	EntityPhysics 		physics[ECS_ELEMENT_COUNT];
	BoxD 				hitboxes[ECS_ELEMENT_COUNT];
	EntityHealth 		healths[ECS_ELEMENT_COUNT];
	EntityAI			ais[ECS_ELEMENT_COUNT];
	EntityRenderState	renderStates[ECS_ELEMENT_COUNT];
};

// Helper function for the registry loading function.
static void registerEntityRecord(EntityRegistry *const pRegistry, const EntityRecord2 record) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Registering entity record with ID \"%s\"...", record.entityID.pBuffer);
	assert(pRegistry);
	
	size_t hashIndex = stringHash(record.entityID, pRegistry->recordCount);
	for (size_t i = 0; i < pRegistry->recordCount; ++i) {
		if (stringIsNull(pRegistry->pRecords[hashIndex].entityID)) {
			pRegistry->pRecords[hashIndex] = record;
			return;
		} else {
			hashIndex += 1;
			hashIndex %= pRegistry->recordCount;
		}
	}
	
	logMsg(loggerGame, LOG_LEVEL_ERROR, "Registering entity record: could not register entity record with ID \"%s\"...", record.entityID.pBuffer);
}

// Helper function for the registry loading function.
static EntityAI findEntityAI(const String string) {
	if (stringIsNull(string)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Finding entity AI: string ID is null.");
		return entityAINone;
	}
	
	#define ENTITY_AI_COUNT 2
	const EntityAI entityAIs[ENTITY_AI_COUNT] = {
		[0] = entityAINone, 
		[1] = entityAISlime
	};
	
	// Just use a linear search to find the entity AI, good enough for now.
	for (size_t i = 0; i < ENTITY_AI_COUNT; ++i) {
		if (stringCompare(string, entityAIs[i].id)) {
			return entityAIs[i];
		}
	}
	
	logMsg(loggerGame, LOG_LEVEL_ERROR, "Finding entity AI: no match found with ID \"%s\".", string.pBuffer);
	return entityAINone;
}

static EntityRecord2 findEntityRecord(const EntityComponentSystem ecs, const String entityID) {
	
	size_t hashIndex = stringHash(entityID, ecs->registry.recordCount);
	for (size_t i = 0; i < ecs->registry.recordCount; ++i) {
		if (stringCompare(entityID, ecs->registry.pRecords[i].entityID)) {
			return ecs->registry.pRecords[i];
		} else {
			hashIndex += 1;
			hashIndex %= ecs->registry.recordCount;
		}
	}
	
	logMsg(loggerGame, LOG_LEVEL_ERROR, "Finding entity AI: no match found with ID \"%s\".", entityID.pBuffer);
	return (EntityRecord2){ };
}

static Vector3D resolveCollision(const Vector3D old_position, const Vector3D new_position, const BoxD hitbox, const BoxD wall);

EntityComponentSystem createEntityComponentSystem(void) {
	EntityComponentSystem ecs = heapAlloc(1, sizeof(struct EntityComponentSystem_T));
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity component system: failed to allocate entity component system.");
		return nullptr;
	}
	ecs->stackPosition = MAX_ENTITY_COUNT - 1;
	for (int32_t i = 0; i < MAX_ENTITY_COUNT; ++i) {
		ecs->entityStack[i] = (Entity2)(i + 1);
	}
	return ecs;
}

void deleteEntityComponentSystem(EntityComponentSystem *const pEntityComponentSystem) {
	assert(pEntityComponentSystem);
	if ((*pEntityComponentSystem)->registry.pRecords) {
		(*pEntityComponentSystem)->registry.pRecords = heapFree((*pEntityComponentSystem)->registry.pRecords);
	}
	*pEntityComponentSystem = heapFree(*pEntityComponentSystem);
}

void registerEntities(EntityComponentSystem ecs, const String fgePath) {
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Registering entities...");
	assert(ecs);
	
	EntityRegistry registry = { };
	File file = openFile(fgePath.pBuffer, FMODE_READ, FMODE_NO_UPDATE, FMODE_BINARY);
	
	uint32_t recordCount = 0;
	fileReadData(file, 1, sizeof(recordCount), &recordCount);
	registry.pRecords = heapAlloc(recordCount, sizeof(EntityRecord2));
	if (!registry.pRecords) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Registering entities: failed to allocate record array.");
		return;
	}
	registry.recordCount = recordCount;
	
	for (size_t i = 0; i < registry.recordCount; ++i) {
		EntityRecord2 record = { };
		record.entityID = fileReadString(file, 64);
		
		fileReadData(file, 1, sizeof(record.componentMask), &record.componentMask);
		fileReadData(file, 1, sizeof(record.traits), &record.traits);
		fileReadData(file, 1, sizeof(record.physics), &record.physics);
		fileReadData(file, 1, sizeof(record.hitbox), &record.hitbox);
		fileReadData(file, 1, sizeof(record.health), &record.health);
		
		String aiID = fileReadString(file, 64);
		record.ai = findEntityAI(aiID);
		deleteString(&aiID);
		
		record.textureID = fileReadString(file, 64);
		fileReadData(file, 1, sizeof(record.textureDimensions), &record.textureDimensions);
		
		registerEntityRecord(&registry, record);
	}
	
	closeFile(&file);
	ecs->registry = registry;
	logMsg(loggerGame, LOG_LEVEL_VERBOSE, "Registered entities.");
}

Entity2 createEntity(EntityComponentSystem ecs, const EntityCreateInfo createInfo) {
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity: entity component system is null.");
		return (Entity2){ };
	}
	
	if (createInfo.componentMask == 0) {
		logMsg(loggerGame, LOG_LEVEL_WARNING, "Creating entity: component mask is zero.");
	}

	Entity2 entity = { };
	if (ecs->stackPosition > 0) {
		entity = ecs->entityStack[ecs->stackPosition--];
	} else {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity: no entity slots available in entity component system %p.", ecs);
		return (Entity2){ };
	}
	
	ecs->slots[entity] = true;
	ecs->componentMasks[entity] = createInfo.componentMask;
	
	if (createInfo.componentMask & ECMP_TRAITS) {
		ecs->traits[entity] = createInfo.traits;
	}
	
	if (createInfo.componentMask & ECMP_PHYSICS) {
		ecs->physics[entity] = createInfo.physics;
	}
	
	if (createInfo.componentMask & ECMP_HITBOX) {
		ecs->hitboxes[entity] = createInfo.hitbox;
	}
	
	if (createInfo.componentMask & ECMP_HEALTH) {
		ecs->healths[entity] = (EntityHealth){
			.currentHP = createInfo.health.currentHP == 0 ? createInfo.health.maxHP : createInfo.health.currentHP,
			.maxHP = createInfo.health.maxHP,
			.invincible = false,
			.iFrameTimer = 0
		};
	}
	
	// EntityRenderState requires EntityPhysics and hitbox
	// TODO: make hitbox optional, wireframe is only rendered if there is a hitbox.
	if (createInfo.componentMask & (ECMP_PHYSICS | ECMP_HITBOX | ECMP_RENDER)) {
		const RenderObjectLoadInfo renderObjectLoadInfo = {
			.textureID = createInfo.textureID,
			.quadCount = 1, // TODO: load wireframe if debug menu is enabled.
			.pQuadLoadInfos = (QuadLoadInfo[2]){
				{
					.quadType = QUAD_TYPE_MAIN,
					.initPosition = createInfo.physics.position,
					.quadDimensions = createInfo.textureDimensions,
					.initAnimation = 2,
					.color = COLOR_WHITE
				}, {
					.quadType = QUAD_TYPE_WIREFRAME,
					.initPosition = createInfo.physics.position,
					.quadDimensions = boxD2F(createInfo.hitbox),
					.initAnimation = 0,
					.color = COLOR_RED // TODO: select color green if entity is the player.
				}
			}
		};
		ecs->renderStates[entity].renderObjectHandle = loadRenderObject(renderObjectLoadInfo);
		if (!renderObjectExists(ecs->renderStates[entity].renderObjectHandle)) {
			logMsg(loggerGame, LOG_LEVEL_ERROR, "Creating entity: failed to load render object.");
		}
		ecs->renderStates[entity].wireframeHandle = -1;
	}
	
	return entity;
}

Entity2 loadEntity2(EntityComponentSystem ecs, const EntityLoadInfo loadInfo) {
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Loading entity: entity component system is null.");
		return (Entity2){ };
	}
	
	const EntityRecord2 record = findEntityRecord(ecs, loadInfo.entityID);
	if (stringIsNull(record.entityID)) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Loading entity: could not find entity record with ID \"%s\".", loadInfo.entityID);
	}
	
	const EntityCreateInfo createInfo = {
		.componentMask = record.componentMask,
		.traits = record.traits,
		.physics = (EntityPhysics){
			.maxSpeed = record.physics.maxSpeed,
			.mass = record.physics.mass,
			.position = loadInfo.physics.position,
			.velocity = loadInfo.physics.velocity,
			.acceleration = loadInfo.physics.acceleration,
		},
		.hitbox = record.hitbox,
		.health = (EntityHealth){
			.currentHP = loadInfo.health.currentHP,
			.maxHP = record.health.maxHP
		},
		.textureID = record.textureID,
		.textureDimensions = record.textureDimensions
	};
	return createEntity(ecs, createInfo);
}

void deleteEntity(EntityComponentSystem ecs, Entity2 *const pEntity) {
	assert(pEntity);
	
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: entity component system is null.");
		return;
	}
	
	if (*pEntity < 1 || *pEntity >= ECS_ELEMENT_COUNT) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: entity handle %i is invalid.", *pEntity);
		return;
	} else if (ecs->slots[*pEntity]) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: entity %i does not exist.", *pEntity);
		return;
	}
	
	if (ecs->stackPosition >= 0 && (ecs->stackPosition + 1) < MAX_ENTITY_COUNT) {
		ecs->slots[*pEntity] = false;
		ecs->componentMasks[*pEntity] = 0U;
		ecs->entityStack[++ecs->stackPosition] = *pEntity;
		*pEntity = (Entity2){ };
	} else {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Deleting entity: stack position %i in entity component system %p is invalid.", ecs->stackPosition, ecs);
	}
}

void executeSystem(EntityComponentSystem ecs, System system) {
	if (!ecs) {
		logMsg(loggerGame, LOG_LEVEL_ERROR, "Executing system: entity component system is null.");
		return;
	}
	
	if (system.componentMask == 0) {
		logMsg(loggerGame, LOG_LEVEL_WARNING, "Executing system: system component mask is zero.");
	}
	
	for (int32_t i = 1; i < ECS_ELEMENT_COUNT; ++i) {
		if (ecs->slots[i] && ecs->componentMasks[i] & system.componentMask) {
			system.functor(ecs, (Entity2)i);
		}
	}
}

void doEntityPhysics(EntityComponentSystem ecs, Entity2 entity) {
	EntityPhysics *const pPhysics = &ecs->physics[entity];
	const BoxD hitbox = ecs->hitboxes[entity];
	
	// Compute and apply kinetic friction.
	// Points in the opposite direction of movement (i.e. velocity), applied to acceleration.
	const double frictionCoefficient = pPhysics->position.z == 1.0 ? fmin(0.5 * pPhysics->maxSpeed, magnitude(pPhysics->velocity)) : 0.0;
	const Vector3D friction = mulVec(normVec(pPhysics->velocity), -frictionCoefficient);
	pPhysics->acceleration = addVec(pPhysics->acceleration, friction);

	// Apply acceleration to velocity.
	const Vector3D previousVelocity = pPhysics->velocity;
	pPhysics->velocity = addVec(pPhysics->velocity, pPhysics->acceleration);
	
	// Compute and apply capped speed.
	const double cappedSpeed = fmin(magnitude(pPhysics->velocity), pPhysics->maxSpeed);
	pPhysics->velocity = normVec(pPhysics->velocity);
	pPhysics->velocity = mulVec(pPhysics->velocity, cappedSpeed);

	// Update entity position.
	const Vector3D previousPosition = pPhysics->position;
	const Vector3D positionStep = pPhysics->velocity;
	Vector3D nextPosition = addVec(previousPosition, positionStep);

	// The square of the distance of the currently selected new position from the old position.
	// This variable is used to track which resolved new position is the shortest from the entity.
	// The squared length is used instead of the real length because it is only used for comparison.
	double step_length_squared = SQUARE(positionStep.x) + SQUARE(positionStep.y) + SQUARE(positionStep.z); 

	for (unsigned int i = 0; i < currentArea.pRooms[currentArea.currentRoomIndex].wallCount; ++i) {
		
		const BoxD wall = currentArea.pRooms[currentArea.currentRoomIndex].pWalls[i];

		Vector3D resolved_position = resolveCollision(previousPosition, nextPosition, hitbox, wall);
		Vector3D resolved_step = subVec(resolved_position, previousPosition);
		const double resolved_step_length_squared = SQUARE(resolved_step.x) + SQUARE(resolved_step.y) + SQUARE(resolved_step.z);

		if (resolved_step_length_squared < step_length_squared) {
			nextPosition = resolved_position;
			step_length_squared = resolved_step_length_squared;
		}
	}

	// Update entity physics to final position, velocity, and acceleration.
	pPhysics->position = nextPosition;
	pPhysics->velocity = subVec(pPhysics->position, previousPosition);
	pPhysics->acceleration = subVec(pPhysics->velocity, previousVelocity);
}

const System physicsSystem = {
	.componentMask = ECMP_PHYSICS | ECMP_HITBOX,
	.functor = doEntityPhysics
};

// Collision with other harmful entities, triggering invincibility frames.
void doEntityDamage(EntityComponentSystem ecs, Entity2 entity) {
	EntityHealth *const pHealth = &ecs->healths[entity];
	
	
	
	if (getTimeMS() - pHealth->iFrameTimer >= 1500) {
		pHealth->invincible = false;
	}
}

const System damageSystem = {
	.componentMask = ECMP_PHYSICS | ECMP_HITBOX | ECMP_HEALTH,
	.functor = doEntityDamage
};

static Vector3D resolveCollision(const Vector3D old_position, const Vector3D new_position, const BoxD hitbox, const BoxD wall) {
	/*
	Collision detection and correction works by transforming 
	the entity's velocity vector into a linear function:
	
	y = mx + b, { s < x < e }, where
	
	x is the independent variable and represents the entity's position in the x-axis,
	y is the dependent variable and represents the entity's position in the y-axis,
	m is the slope of the function and the ratio of y component of the velocity to the x component,
	b is the y-intercept of the function,
	s is the lower bound of the domain of the function and represents 
		the starting point of the entity's travel along the x-axis, and
	e is the upper bound of the domain of the function and represents
		the ending point of the entity's travel along the x-axis.
	
	Four such functions can be generated, one for each corner of the hitbox, 
	using x as the independent variable and y as the dependent variable. 
	Four more can be generated swapping the axes,
	which effectively checks for collision in the y-direction instead of the x-direction.
	One function is generated for each corner of the entity's hitbox, for these are the
	extremities of the entity's body. With the extremities, collision can be checked
	with the following independently sufficient conditions:
	
	1. Any extremity is both above the lower bound and below the upper bound of the wall, OR
	2. The lower extremity is above the lower bound of the wall, and the upper extremity
		is below the upper bound of the wall.
	
	Any of these conditions being fulfilled means that collision has happened.
	
	There are two extremities when checking collision on a particular axis. 
	Though there are four corners, only two are at the fore of the entity's movement
	and can collide with a wall that the entity may move into.
	Because of this, two linear functions are generated, one for each of those
	two corners--one upper function and one lower function.
	In order to generate these functions, first the base linear function
	must be transposed such that the origin is at the old position of the entity:
	
	y - old_position.y = m(x - old_position.x).
	
	This is also known as 'point-slope form' of a linear function.
	The function then can be rearranged back into 'slope-intercept form':
	
	y = m(x - old_position.x) + old_position.y.
	
	Keep in mind that the domain of the function is also transposed:
	
	y - old_position.y = m(x - old_position.x) { s < x - old_position.x < e }; 
	y = m(x - old_position.x) + old_position.y { s + old_position.x < x < e + old_position.x }.
	
	The function now maps to the travel of the entity when graphed:
	
	+y |
	   |
	 6 |      B
	   |     /
	 4 |    /
	   |   /
	 2 |  A
	   |
	   O---------------------------
	   	2 4 6 8 |    |    |    | +x
	   		   10   15   20   25
	
	The capital A represents the old position of the entity, and
	the capital B represents the projected new position of the entity.
	
	This process can be repeated for each of the two extremities to produce
	the lower and upper functions. When moving right, which is the +x direction,
	the +x side of the hitbox will be the outward extremity, and is represented with hitbox.x2.
	When moving left, which is the -x direction, the -x side of 
	the hitbox will be the outward extremity, and is represented with hitbox.x1.
	The lower extremity will always use the lower side of the hitbox, which is
	represented with hitbox.y1. The upper extremity will always use the upper side
	of the hitbox, which is represented with hitbox.y2.
	
	Assuming moving right for convenience, the lower function is created like this:
	
	y - hitbox.y1 = m((x - hitbox.x2) - old_position.x) + old_position.y 
		{ s + old_position.x < x - hitbox.x2 < e + old_position.x };
	y = m(x - hitbox.x2 - old_position.x) + old_position.y + hitbox.y1 
		{ s + old_position.x + hitbox.x2 < x < e + old_position.x + hitbox.x2 }.
	
	Similarly, the upper function is created like this:
	
	y - hitbox.y2 = m((x - hitbox.x2) - old_position.x) + old_position.y 
		{ s + old_position.x < x - hitbox.x2 < e + old_position.x };
	y = m(x - hitbox.x2 - old_position.x) + old_position.y + hitbox.y2 
		{ s + old_position.x + hitbox.x2 < x < e + old_position.x + hitbox.x2 }.
	
	When x is set to an x-position, then y equals the y-position along the line of travel.
	Theoretically, this extends out to infinity, but we are only concerned with the entity's
	movement in a single step, which is not infinite; thus, we restrict the function to a domain
	representing the start and end point of the entity's travel.
	Now it is possible to check for collision, by finding the exact y-position of each of 
	the extremities in the entity's travel, and then comparing these y-positions to the
	upper and lower y-positions of the wall, using the above conditions:
	
	if wall.y1 < y_lower < wall.y2 OR wall.y1 < y_upper < wall.y2 
		OR (y_lower < wall.y1 AND y_upper > wall.y2), then
	
		the entity collides.
	*/

	// This parameter determines how much the entity is slowed down when sliding against a wall.
	static const double friction_factor = 0.75;

	const Vector3D position_step = subVec(new_position, old_position);
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
				resolved_position = addVec(old_position, resolved_step);
			} else {
				resolved_position = new_position;
				resolved_position.x = wall.x1 - hitbox.x2;
				resolved_position.y = slope_y * (resolved_position.x - old_position.x) + old_position.y;
			}
			goto correct;
		}
	} else if (position_step.x < 0.0) {
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
				resolved_position = addVec(old_position, resolved_step);
			} else {
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
				resolved_position = addVec(old_position, resolved_step);
			} else {
				resolved_position = new_position;
				resolved_position.y = wall.y1 - hitbox.y2;
				resolved_position.x = slope_x * (resolved_position.y - old_position.y) + old_position.x;
			}
			goto correct;
		}
	} else if (position_step.y < 0.0) {
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
				resolved_position = addVec(old_position, resolved_step);
			} else {
				resolved_position = new_position;
				resolved_position.y = wall.y2 - hitbox.y1;
				resolved_position.x = slope_x * (resolved_position.y - old_position.y) + old_position.x;
			}
			goto correct;
		}
	}

skip: // Jump to this label if the movement step does not need to be corrected, i.e. there is no collision.
	return new_position;

correct: // Jump to this label if the movement step needs to be corrected, i.e. there is collision.
	return resolved_position;
}