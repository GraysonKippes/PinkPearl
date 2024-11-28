#include "EntitySpawner.h"

#include "entity_manager.h"
#include "EntityRegistry.h"

#include "util/Random.h"

void entitySpawnerReload(EntitySpawner *const pEntitySpawner) {
	if (!pEntitySpawner) {
		return;
	}
	//pEntitySpawner->spawnCounter = random(pEntitySpawner->minSpawnCount, pEntitySpawner->maxSpawnCount);
	pEntitySpawner->spawnCounter = 3;
}

void entitySpawnerSpawnEntities(EntitySpawner *const pEntitySpawner) {
	if (!pEntitySpawner) {
		return;
	}
	
	if (pEntitySpawner->spawnCounter <= 0) {
		return;
	}
	
	for (int i = 0; i < pEntitySpawner->spawnCounter; ++i) {
		loadEntity(pEntitySpawner->entityID, makeVec3D(5.5, 2.0, 0.5), zeroVec3D);
	}
	pEntitySpawner->spawnCounter = 0;
}