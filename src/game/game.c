#include "game.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "glfw/input_manager.h"
#include "log/logging.h"
#include "render/area_render_state.h"
#include "render/render_object.h"
#include "render/renderer.h"
#include "render/texture_state.h"
#include "util/bit.h"
#include "util/time.h"

#include "game_state.h"
#include "area/fgm_file_parse.h"
#include "entity/entity_manager.h"
#include "entity/entity_registry.h"

// TODO - remove/replace with struct of booleans/ints
static game_state_bitfield_t game_state_bitfield = 0;

area_t current_area = { };
static int current_room_index = 0;

static int playerEntityHandle;

void start_game(void) {

	current_area = parse_fga_file("test");
	areaRenderStateReset(&globalAreaRenderState, current_area, current_area.rooms[current_room_index]);

	String entityID = newString(64, "pearl");
	playerEntityHandle = loadEntity(entityID, (Vector3D){ 0.0, 0.0, -32.0 }, (Vector3D){ 0.0, 0.0, 0.0 });
	if (!validateEntityHandle(playerEntityHandle)) {
		logMsgF(LOG_LEVEL_ERROR, "Failed to load entity \"%s\".", entityID.buffer);
	}
	deleteString(&entityID);
}

void tick_game(void) {

	const bool move_up_pressed = is_input_pressed_or_held(GLFW_KEY_W);		// UP
	const bool move_left_pressed = is_input_pressed_or_held(GLFW_KEY_A);	// LEFT
	const bool move_down_pressed = is_input_pressed_or_held(GLFW_KEY_S);	// DOWN
	const bool move_right_pressed = is_input_pressed_or_held(GLFW_KEY_D);	// RIGHT

	if (game_state_bitfield & (uint32_t)GAME_STATE_SCROLLING) {
		if (!areaRenderStateIsScrolling(globalAreaRenderState)) {
			game_state_bitfield &= ~((uint32_t)GAME_STATE_SCROLLING);
		}
		else {
			//settle_render_positions();
			return;
		}
	}

	entity_t *pPlayerEntity = nullptr;
	int result = getEntity(playerEntityHandle, &pPlayerEntity);
	if (pPlayerEntity == nullptr || result != 0) {
		return;
	}

	static const double max_speed = 0.24;	// Tiles / s
	static const double acceleration_constant = 1.8; // Tiles / s^2
	const double accelerated_speed = (getTimeMS() - pPlayerEntity->transform.last_stationary_time) * 0.001 * acceleration_constant;
	const double speed = fmin(accelerated_speed, max_speed);

	pPlayerEntity->transform.velocity.x = 0.0;
	pPlayerEntity->transform.velocity.y = 0.0;
	pPlayerEntity->transform.velocity.z = 0.0;
	
	const unsigned int currentAnimation = renderObjectGetAnimation(pPlayerEntity->render_handle, 0);
	unsigned int nextAnimation = currentAnimation;

	if (move_up_pressed && !move_down_pressed) {
		pPlayerEntity->transform.velocity.y = 1.0;
		nextAnimation = 1;
	}
	
	if (!move_up_pressed && move_down_pressed) {
		pPlayerEntity->transform.velocity.y = -1.0;
		nextAnimation = 5;
	}
	
	if (move_left_pressed && !move_right_pressed) {
		pPlayerEntity->transform.velocity.x = -1.0;
		nextAnimation = 7;
	}
	
	if (!move_left_pressed && move_right_pressed) {
		pPlayerEntity->transform.velocity.x = 1.0;
		nextAnimation = 3;
	}

	pPlayerEntity->transform.velocity = vector3D_normalize(pPlayerEntity->transform.velocity);
	pPlayerEntity->transform.velocity = vector3D_scalar_multiply(pPlayerEntity->transform.velocity, speed);
	
	if (nextAnimation != currentAnimation) {
		renderObjectSetAnimation(pPlayerEntity->render_handle, 0, nextAnimation);
	}

	tickEntities();

	const direction_t travel_direction = test_room_travel(pPlayerEntity->transform.position, current_area, current_room_index);
	if ((int)travel_direction > 0) {

		const Room current_room = current_area.rooms[current_room_index];
		const Offset room_offset = direction_offset(travel_direction);
		const Offset next_room_position = offset_add(current_room.position, room_offset);

		const Room *next_room_ptr = nullptr;
		const bool result = area_get_room_ptr(current_area, next_room_position, &next_room_ptr);

		if (result && next_room_ptr != nullptr) {
			current_room_index = area_get_room_index(current_area, next_room_position);
			game_state_bitfield |= (uint32_t)GAME_STATE_SCROLLING;
			areaRenderStateSetNextRoom(&globalAreaRenderState, *next_room_ptr);
		}
	}
}
