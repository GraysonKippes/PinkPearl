#ifndef ENTITY_SPAWNER_H
#define ENTITY_SPAWNER_H

#include "util/string.h"

// Controls how often an object (e.g. entity spawner) in a room reloads.
typedef enum ReloadMode {
	RELOAD_NEVER = 0,					// Object will never respawn.
	RELOAD_AFTER_REFRESH = 1,			// Object will only respawn after room refresh.
	RELOAD_ALWAYS = 2,					// Object will always respawn when it is reentered.
	RELOAD_ALWAYS_UNTIL_ROOM_DEFEAT = 3	// Object will always respawn until the room is beaten/completed.
} ReloadMode;

// Controls the spawning of a single entity/enemy in a room.
typedef struct EntitySpawner {
	
	// String ID of the entity type to spawn.
	String entityID;
	
	// Indicates when/how often the spawner is reloaded.
	ReloadMode reloadMode;
	
	// The number of entities that will be spawned in the current load.
	// Each time the spawner is (re)loaded, a random number is generated between minSpawnCount and maxSpawnCount, inclusive.
	// This counter is counted down as the entities are spawned until it reaches zero.
	int spawnCounter;
	
	// The minimum number of entities that can be spawned when this spawner is loaded.
	int minSpawnCount;
	
	// The maximum number of entities that can be spawned when this spawner is loaded.
	int maxSpawnCount;
	
} EntitySpawner;

void entitySpawnerReload(EntitySpawner *const pEntitySpawner);

void entitySpawnerSpawnEntities(EntitySpawner *const pEntitySpawner);

#endif	// ENTITY_SPAWNER_H