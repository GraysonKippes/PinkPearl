#include "game.h"

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include "glfw/InputManager.h"
#include "log/Logger.h"
#include "render/RenderManager.h"
#include "render/vulkan/TextureState.h"
#include "util/time.h"
#include "area/fgm_file_parse.h"
#include "entity/entity_manager.h"
#include "entity/EntityRegistry.h"
#include "entity/EntitySpawner.h"

#define DEBUG_TEXT_HANDLE_COUNT 3

// Stores keybinds for playing the game.
typedef struct GameControls {
	int pauseGame;
	int debugMenu;
	int moveUp;
	int moveLeft;
	int moveDown;
	int moveRight;
	int useItemLeft;
	int useItemRight;
} GameControls;

// Stores render state for various visual aspects of the game, including player HUD and debug menu.
typedef struct GameRenderState {
	
	// Player HUD
	int32_t heartsHandle; // Player hit points, represented as hearts.
	int32_t slotsHandle; // Equipped item slots.
	int32_t pauseTextHandle; // Pause screen text.
	
	// Debug Menu
	bool debugMenuEnabled;
	int32_t debugTextHandles[DEBUG_TEXT_HANDLE_COUNT];
	
} GameRenderState;

static GameState gameState = { };
static GameControls controls = {
	.pauseGame = GLFW_KEY_ESCAPE,
	.debugMenu = GLFW_KEY_F3,
	.moveUp = GLFW_KEY_W,
	.moveLeft = GLFW_KEY_A,
	.moveDown = GLFW_KEY_S,
	.moveRight = GLFW_KEY_D,
	.useItemLeft = MOUSE_BUTTON_LEFT,
	.useItemRight = MOUSE_BUTTON_RIGHT
};
static GameRenderState renderState = { };

// The area that the player is currently in.
Area currentArea = { };

// The handle to the player entity.
static int playerEntityHandle = -1;

static void pauseGame(GameState *const pGameState, GameRenderState *const pGameRenderState);

static void toggleDebugMenu(GameRenderState *const pGameRenderState);

static void addPlayerHP(Entity *const pPlayerEntity, const int32_t addAmount, GameState *const pGameState, const GameRenderState gameRenderState);

void start_game(void) {
	
	currentArea = readAreaData("test");
	areaRenderStateReset(&currentArea, currentArea.pRooms[currentArea.currentRoomIndex]);
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
		.textureID = makeStaticString("gui/heart3"),
		.quadCount = 3,
		.pQuadLoadInfos = (QuadLoadInfo[3]){
			{
				.quadType = QUAD_TYPE_GUI,
				.initPosition = makeVec3D(-11.5, 7.25, 3.0),
				.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
				.initAnimation = 0,
				.initCell = 3,
				.color = COLOR_WHITE
			}, {
				.quadType = QUAD_TYPE_GUI,
				.initPosition = makeVec3D(-11.0, 7.25, 3.0),
				.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
				.initAnimation = 0,
				.initCell = 3,
				.color = COLOR_WHITE
			}, {
				.quadType = QUAD_TYPE_GUI,
				.initPosition = makeVec3D(-10.5, 7.25, 3.0),
				.quadDimensions = (BoxF){ .x1 = -0.25F, .y1 = -0.25F, .x2 = 0.25F, .y2 = 0.25F },
				.initAnimation = 0,
				.initCell = 3,
				.color = COLOR_WHITE
			}
		}
	};
	renderState.heartsHandle = loadRenderObject(loadInfoHearts);
	
	const RenderObjectLoadInfo loadInfoSlots = {
		.textureID = makeStaticString("gui/slots"),
		.quadCount = 1,
		.pQuadLoadInfos = (QuadLoadInfo[1]){
			{
				.quadType = QUAD_TYPE_GUI,
				.initPosition = makeVec3D(10.25, 7.0, 3.0),
				.quadDimensions = (BoxF){ .x1 = -1.75F, .y1 = -0.5F, .x2 = 1.75F, .y2 = 0.5F },
				.initAnimation = 0,
				.initCell = 0,
				.color = COLOR_WHITE
			}
		}
	};
	renderState.slotsHandle = loadRenderObject(loadInfoSlots);
}

void endGame(void) {
	deleteArea(&currentArea);
}

void tick_game(void) {

	tickRenderManager();

	if (isInputPressed(controls.debugMenu)) {
		toggleDebugMenu(&renderState);
	}

	if (isInputPressed(controls.pauseGame)) {
		pauseGame(&gameState, &renderState);
	}

	if (gameState.paused || gameState.gameOver) {
		return;
	}

	if (gameState.scrolling) {
		if (!areaIsScrolling(currentArea)) {
			return;
		}
		gameState.scrolling = false;
		unloadImpersistentEntities();
	}

	const bool move_up_pressed = isInputPressedOrHeld(controls.moveUp);		// UP
	const bool move_left_pressed = isInputPressedOrHeld(controls.moveLeft);	// LEFT
	const bool move_down_pressed = isInputPressedOrHeld(controls.moveDown);	// DOWN
	const bool move_right_pressed = isInputPressedOrHeld(controls.moveRight);	// RIGHT

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
	
	if (renderState.debugMenuEnabled) {
		writeRenderText(renderState.debugTextHandles[0], "P %.2f, %.2f", pPlayerEntity->physics.position.x, pPlayerEntity->physics.position.y);
		writeRenderText(renderState.debugTextHandles[1], "V %.3f, %.3f", pPlayerEntity->physics.velocity.x, pPlayerEntity->physics.velocity.y);
		writeRenderText(renderState.debugTextHandles[2], "A %.3f, %.3f", pPlayerEntity->physics.acceleration.x, pPlayerEntity->physics.acceleration.y);
	}
	
	for (int32_t entityHandle = 0; entityHandle < maxNumEntities; ++entityHandle) {
		if (entityHandle == playerEntityHandle) {
			continue;
		}
		
		Entity *pEntity = nullptr;
		if (getEntity(entityHandle, &pEntity) != 0) {
			continue;
		}
		
		if (entityCollision(*pPlayerEntity, *pEntity)) {
			if (!pPlayerEntity->invincible) {
				addPlayerHP(pPlayerEntity, -1, &gameState, renderState);
			}
			entityTriggerInvincibility(pPlayerEntity);
		}
	}

	const CardinalDirection travelDirection = test_room_travel(pPlayerEntity->physics.position, currentArea, currentArea.currentRoomIndex);
	gameState.scrolling = areaSetNextRoom(&currentArea, travelDirection);
}

GameState getGameState(void) {
	return gameState;
}

static void pauseGame(GameState *const pGameState, GameRenderState *const pGameRenderState) {
	assert(pGameState);
	pGameState->paused = !pGameState->paused;
	if (pGameState->paused) {
		pGameRenderState->pauseTextHandle = loadRenderText(makeStaticString("Paused"), makeVec3D(-1.5, 0.25, 3.0), COLOR_WHITE);
	} else {
		unloadRenderObject(&pGameRenderState->pauseTextHandle);
	}
}

static void toggleDebugMenu(GameRenderState *const pGameRenderState) {
	assert(pGameRenderState);
	pGameRenderState->debugMenuEnabled = !pGameRenderState->debugMenuEnabled;
	if (pGameRenderState->debugMenuEnabled) {
		pGameRenderState->debugTextHandles[0] = loadRenderText(makeStaticString("Position Position"), makeVec3D(-11.5, -6.25, 4.0), COLOR_PINK);
		pGameRenderState->debugTextHandles[1] = loadRenderText(makeStaticString("Velocity Velocity"), makeVec3D(-11.5, -6.75, 4.0), COLOR_PINK);
		pGameRenderState->debugTextHandles[2] = loadRenderText(makeStaticString("Acceleration Acce"), makeVec3D(-11.5, -7.25, 4.0), COLOR_PINK);
		drawEntityHitboxes();
		areaLoadWireframes(&currentArea);
	} else {
		unloadRenderObject(&pGameRenderState->debugTextHandles[0]);
		unloadRenderObject(&pGameRenderState->debugTextHandles[1]);
		unloadRenderObject(&pGameRenderState->debugTextHandles[2]);
		undrawEntityHitboxes();
		areaUnloadWireframes(&currentArea);
	}
}

static void addPlayerHP(Entity *const pPlayerEntity, const int32_t addAmount, GameState *const pGameState, const GameRenderState gameRenderState) {
	assert(pPlayerEntity);
	
	const int32_t newHP = pPlayerEntity->currentHP + addAmount;
	if (newHP > pPlayerEntity->maxHP) {
		pPlayerEntity->currentHP = pPlayerEntity->maxHP;
	} else if (newHP <= 0) {
		pPlayerEntity->currentHP = 0;
		pGameState->gameOver = true;
	} else {
		pPlayerEntity->currentHP = newHP;
	}
	
	// Update HP bar in the player HUD.
	int32_t fillHP = pPlayerEntity->currentHP;
	for (int32_t quadIndex = 0; quadIndex < 3; ++quadIndex) {
		int32_t imageIndex = 0;
		if (fillHP >= 3) {
			imageIndex = 3;
		} else if (fillHP > 0) {
			imageIndex = fillHP;
		}
		renderObjectSetQuadImage(gameRenderState.heartsHandle, quadIndex, imageIndex);
		fillHP -= 3;
	}
}