#include "EntitySpawner.h"

#include "entity_manager.h"
#include "EntityRegistry.h"

#include "util/Random.h"

void entitySpawnerReload(EntitySpawner *const pEntitySpawner) {
	if (!pEntitySpawner) {
		return;
	}
	pEntitySpawner->spawnCounter = random(pEntitySpawner->minSpawnCount, pEntitySpawner->maxSpawnCount);
}

void entitySpawnerSpawnEntities(EntitySpawner *const pEntitySpawner) {
	if (!pEntitySpawner) {
		return;
	}
	
	if (pEntitySpawner->spawnCounter <= 0) {
		return;
	}
	
	for (int i = 0; i < pEntitySpawner->spawnCounter; ++i) {
		loadEntity(pEntitySpawner->entityID, (Vector3D){ 5.5, 2.0, -32.0 }, (Vector3D){ 0.0, 0.0, 0.0 });
	}
	pEntitySpawner->spawnCounter = 0;
}