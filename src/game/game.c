#include "game.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "glfw/input_manager.h"
#include "log/Logger.h"
#include "render/area_render_state.h"
#include "render/render_object.h"
#include "render/renderer.h"
#include "render/texture_state.h"
#include "util/time.h"

#include "game_state.h"
#include "area/fgm_file_parse.h"
#include "entity/entity_manager.h"
#include "entity/EntityRegistry.h"
#include "entity/EntitySpawner.h"

// Stores information about the Pink Pearl game, such as whether or not the game is paused.
static GameState gameState = { };

// The area that the player is currently in.
Area currentArea = { };

// The handle to the player entity.
static int playerEntityHandle;

void start_game(void) {

	currentArea = readAreaData("test");
	areaRenderStateReset(&globalAreaRenderState, currentArea, currentArea.pRooms[currentArea.currentRoomIndex]);

	String playerEntityID = { .length = 5, .capacity = 6, .pBuffer = "pearl" };
	playerEntityHandle = loadEntity(playerEntityID, (Vector3D){ 0.0, 0.0, -32.0 }, (Vector3D){ 0.0, 0.0, 0.0 });
	
	// Test entity spawner.
	EntitySpawner testEntitySpawner = {
		.entityID = (String){ .length = 5, .capacity = 6, .pBuffer = "slime" },
		.reloadMode = RELOAD_AFTER_REFRESH,
		.spawnCounter = 0,
		.minSpawnCount = 2,
		.maxSpawnCount = 5
	};
	
	//entitySpawnerReload(&testEntitySpawner);
	testEntitySpawner.spawnCounter = 5;
	entitySpawnerSpawnEntities(&testEntitySpawner);
}

void tick_game(void) {

	const bool move_up_pressed = is_input_pressed_or_held(GLFW_KEY_W);		// UP
	const bool move_left_pressed = is_input_pressed_or_held(GLFW_KEY_A);	// LEFT
	const bool move_down_pressed = is_input_pressed_or_held(GLFW_KEY_S);	// DOWN
	const bool move_right_pressed = is_input_pressed_or_held(GLFW_KEY_D);	// RIGHT

	if (gameState.scrolling) {
		if (!areaRenderStateIsScrolling(globalAreaRenderState)) {
			gameState.scrolling = false;
			unloadImpersistentEntities();
		}
		else {
			//settle_render_positions();
			return;
		}
	}

	Entity *pPlayerEntity = nullptr;
	int result = getEntity(playerEntityHandle, &pPlayerEntity);
	if (pPlayerEntity == nullptr || result != 0) {
		return;
	}
	
	//static const double speed = 0.24;
	static const double accelerationMagnitude = 0.24;

	pPlayerEntity->physics.acceleration = zeroVector3D;
	
	const unsigned int currentAnimation = renderObjectGetAnimation(pPlayerEntity->renderHandle, 0);
	unsigned int nextAnimation = currentAnimation;

	if (move_up_pressed && !move_down_pressed) {
		pPlayerEntity->physics.acceleration.y = 1.0;
		nextAnimation = 1;
	}
	
	if (!move_up_pressed && move_down_pressed) {
		pPlayerEntity->physics.acceleration.y = -1.0;
		nextAnimation = 5;
	}
	
	if (move_left_pressed && !move_right_pressed) {
		pPlayerEntity->physics.acceleration.x = -1.0;
		nextAnimation = 7;
	}
	
	if (!move_left_pressed && move_right_pressed) {
		pPlayerEntity->physics.acceleration.x = 1.0;
		nextAnimation = 3;
	}

	pPlayerEntity->physics.acceleration = vector3D_normalize(pPlayerEntity->physics.acceleration);
	pPlayerEntity->physics.acceleration = vector3D_scalar_multiply(pPlayerEntity->physics.acceleration, accelerationMagnitude);
	
	if (nextAnimation != currentAnimation) {
		renderObjectSetAnimation(pPlayerEntity->renderHandle, 0, nextAnimation);
	}

	tickEntities();

	const CardinalDirection travelDirection = test_room_travel(pPlayerEntity->physics.position, currentArea, currentArea.currentRoomIndex);
	if (travelDirection != DIRECTION_NONE) {

		const Room current_room = currentArea.pRooms[currentArea.currentRoomIndex];
		const Offset room_offset = direction_offset(travelDirection);
		const Offset next_room_position = offset_add(current_room.position, room_offset);

		Room *pNextRoom = nullptr;
		const bool result = areaGetRoom(currentArea, next_room_position, &pNextRoom);
		if (result && pNextRoom != nullptr) {
			gameState.scrolling = true;
			currentArea.currentRoomIndex = pNextRoom->id;
			areaRenderStateSetNextRoom(&globalAreaRenderState, *pNextRoom);
		}
	}
}
