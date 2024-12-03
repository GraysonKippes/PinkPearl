#include "game.h"

#include <assert.h>
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

// Move from global state to struct.
static int32_t heartsHandle = -1;
static int32_t slotsHandle = -1;
static int32_t pausedHandle = -1;

#define DEBUG_TEXT_HANDLE_COUNT 3
static bool debugMenuEnabled = false;
static int32_t debugTextHandles[DEBUG_TEXT_HANDLE_COUNT] = { -1, -1, -1 };

void pauseGame(GameState *const pGameState);

void toggleDebugMenu(void);

void start_game(void) {
	
	currentArea = readAreaData("test");
	areaRenderStateReset(&globalAreaRenderState, currentArea, currentArea.pRooms[currentArea.currentRoomIndex]);
	playerEntityHandle = loadEntity(makeStaticString("pearl"), makeVec3D(0.0, 0.0, 1.0), zeroVec3D);
	
	// Test entity spawner.
	EntitySpawner testEntitySpawner = {
		.entityID = makeStaticString("slime"),
		.reloadMode = RELOAD_AFTER_REFRESH,
		.spawnCounter = 0,
		.minSpawnCount = 1,
		.maxSpawnCount = 3
	};
	
	entitySpawnerReload(&testEntitySpawner);
	entitySpawnerSpawnEntities(&testEntitySpawner);
	
	const RenderObjectLoadInfo loadInfoHearts = {
		.isGUIElement = true,
		.textureID = makeStaticString("gui/heart3"),
		.quadCount = 3,
		.pQuadLoadInfos = (QuadLoadInfo[3]){
			{
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = makeVec3D(-11.5, 7.25, 3.0),
				.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
				.initAnimation = 0,
				.initCell = 0,
				.color = COLOR_WHITE
			}, {
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = makeVec3D(-11.0, 7.25, 3.0),
				.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
				.initAnimation = 0,
				.initCell = 0,
				.color = COLOR_WHITE
			}, {
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = makeVec3D(-10.5, 7.25, 3.0),
				.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
				.initAnimation = 0,
				.initCell = 0,
				.color = COLOR_WHITE
			}
		}
	};
	heartsHandle = loadRenderObject(loadInfoHearts);
	
	const RenderObjectLoadInfo loadInfoSlots = {
		.isGUIElement = true,
		.textureID = makeStaticString("gui/slots"),
		.quadCount = 1,
		.pQuadLoadInfos = (QuadLoadInfo[1]){
			{
				.quadType = QUAD_TYPE_MAIN,
				.initPosition = makeVec3D(10.25, 7.0, 3.0),
				.quadDimensions = (BoxF){ .x1 = -1.75F, .y1 = -0.5F, .x2 = 1.75F, .y2 = 0.5F },
				.initAnimation = 0,
				.initCell = 0,
				.color = COLOR_WHITE
			}
		}
	};
	slotsHandle = loadRenderObject(loadInfoSlots);
}

void tick_game(void) {

	tickRenderManager();

	if (isInputPressed(GLFW_KEY_ESCAPE)) {
		pauseGame(&gameState);
	}

	if (gameState.paused) {
		return;
	}

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
	if (!pPlayerEntity || result != 0) {
		return;
	}
	
	static const double accelerationMagnitude = 0.24;
	pPlayerEntity->physics.acceleration = zeroVec3D;
	const unsigned int currentAnimation = renderObjectGetAnimation(pPlayerEntity->renderHandle, 0);
	unsigned int nextAnimation = currentAnimation;

	if (move_up_pressed && !move_down_pressed) {
		pPlayerEntity->physics.acceleration = unitVec3DPosY;
		nextAnimation = 1;
	}
	
	if (!move_left_pressed && move_right_pressed) {
		pPlayerEntity->physics.acceleration = unitVec3DPosX;
		nextAnimation = 3;
	}
	
	if (!move_up_pressed && move_down_pressed) {
		pPlayerEntity->physics.acceleration = unitVec3DNegY;
		nextAnimation = 5;
	}
	
	if (move_left_pressed && !move_right_pressed) {
		pPlayerEntity->physics.acceleration = unitVec3DNegX;
		nextAnimation = 7;
	}

	pPlayerEntity->physics.acceleration = normVec(pPlayerEntity->physics.acceleration);
	pPlayerEntity->physics.acceleration = mulVec(pPlayerEntity->physics.acceleration, accelerationMagnitude);
	tickEntities();
	
	if (nextAnimation != currentAnimation) {
		renderObjectSetAnimation(pPlayerEntity->renderHandle, 0, nextAnimation);
	}
	
	if (debugMenuEnabled) {
		writeRenderText(debugTextHandles[0], "P %.2f, %.2f", pPlayerEntity->physics.position.x, pPlayerEntity->physics.position.y);
		writeRenderText(debugTextHandles[1], "V %.3f, %.3f", pPlayerEntity->physics.velocity.x, pPlayerEntity->physics.velocity.y);
		writeRenderText(debugTextHandles[2], "A %.3f, %.3f", pPlayerEntity->physics.acceleration.x, pPlayerEntity->physics.acceleration.y);
	}

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

void pauseGame(GameState *const pGameState) {
	assert(pGameState);
	pGameState->paused = !pGameState->paused;
	if (pGameState->paused) {
		pausedHandle = loadRenderText(makeStaticString("Paused"), makeVec3D(-1.5, 0.25, 3.0), COLOR_WHITE);
	} else {
		unloadRenderObject(&pausedHandle);
	}
}

bool isGamePaused(void) {
	return gameState.paused;
}

void toggleDebugMenu(void) {
	debugMenuEnabled = !debugMenuEnabled;
	if (debugMenuEnabled) {
		debugTextHandles[0] = loadRenderText(makeStaticString("Position Position"), makeVec3D(-11.5, 7.25, 4.0), COLOR_PINK);
		debugTextHandles[1] = loadRenderText(makeStaticString("Velocity Velocity"), makeVec3D(-11.5, 6.75, 4.0), COLOR_PINK);
		debugTextHandles[2] = loadRenderText(makeStaticString("Acceleration Acce"), makeVec3D(-11.5, 6.25, 4.0), COLOR_PINK);
	} else {
		unloadRenderObject(&debugTextHandles[0]);
		unloadRenderObject(&debugTextHandles[1]);
		unloadRenderObject(&debugTextHandles[2]);
	}
}
