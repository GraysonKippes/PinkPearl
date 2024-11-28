#include "game.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "glfw/input_manager.h"
#include "log/Logger.h"
#include "render/area_render_state.h"
#include "render/RenderManager.h"
#include "render/vulkan/TextureState.h"
#include "util/time.h"
#include "area/fgm_file_parse.h"
#include "entity/entity_manager.h"
#include "entity/EntityRegistry.h"
#include "entity/EntitySpawner.h"

typedef struct GameState {
	bool paused;
	bool scrolling;
} GameState;

// Stores information about the Pink Pearl game, such as whether or not the game is paused.
static GameState gameState = { };

// The area that the player is currently in.
Area currentArea = { };

// The handle to the player entity.
static int playerEntityHandle = -1;

static bool debugMenuEnabled = false;
static int32_t debugTextHandle = -1;

void toggleDebugMenu(void);

void start_game(void) {

	currentArea = readAreaData("test");
	areaRenderStateReset(&globalAreaRenderState, currentArea, currentArea.pRooms[currentArea.currentRoomIndex]);

	playerEntityHandle = loadEntity(makeStaticString("pearl"), makeVec3D(0.0, 0.0, -32.0), zeroVec3D);
	
	// Test entity spawner.
	EntitySpawner testEntitySpawner = {
		.entityID = makeStaticString("slime"),
		.reloadMode = RELOAD_AFTER_REFRESH,
		.spawnCounter = 0,
		.minSpawnCount = 2,
		.maxSpawnCount = 5
	};
	
	entitySpawnerReload(&testEntitySpawner);
	entitySpawnerSpawnEntities(&testEntitySpawner);
}

void tick_game(void) {

	if (isInputPressed(GLFW_KEY_F3)) {
		toggleDebugMenu();
	}

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
	
	if (debugMenuEnabled) {
		writeRenderText(debugTextHandle, "%.2f", pPlayerEntity->physics.position.x);
	}
	
	//static const double speed = 0.24;
	static const double accelerationMagnitude = 0.24;
	pPlayerEntity->physics.acceleration = zeroVec3D;
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

	pPlayerEntity->physics.acceleration = normVec(pPlayerEntity->physics.acceleration);
	pPlayerEntity->physics.acceleration = mulVec(pPlayerEntity->physics.acceleration, accelerationMagnitude);
	
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

void toggleDebugMenu(void) {
	debugMenuEnabled = !debugMenuEnabled;
	if (debugMenuEnabled) {
		debugTextHandle = loadRenderText(makeStaticString("Position"), makeVec3D(-6.0, 3.75, -32.0), COLOR_WHITE);
	} else {
		unloadRenderObject(&debugTextHandle);
	}
}
