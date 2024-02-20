#include "game.h"

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

#include "glfw/input_manager.h"
#include "log/logging.h"
#include "render/render_object.h"
#include "render/renderer.h"
#include "render/texture_state.h"
#include "render/vulkan/texture_manager.h"
#include "render/vulkan/compute/compute_area_mesh.h"
#include "util/bit.h"
#include "util/time.h"

#include "game_state.h"
#include "area/fgm_file_parse.h"
#include "entity/entity_manager.h"

static game_state_bitfield_t game_state_bitfield = 0;

area_t current_area = { 0 };
static int current_room_index = 0;

static const rect_t player_hitbox = {
	.x1 = -0.375,
	.y1 = -0.5,
	.x2 = 0.375,
	.y2 = 0.5
};

static entity_handle_t player_entity_handle;

void start_game(void) {

	init_entity_manager();
	current_area = parse_fga_file("test");
	upload_room_model(current_area.rooms[0]);
	// TODO - this is test, remove this eventually...
	compute_area_mesh(current_area);

	player_entity_handle = load_entity();
	if (!validate_entity_handle(player_entity_handle)) {
		// TODO - error handling
	}

	entity_t *player_entity_ptr = NULL;
	int result = get_entity_ptr(player_entity_handle, &player_entity_ptr);
	if (player_entity_ptr != NULL || result == 0) {
		player_entity_ptr->hitbox = player_hitbox;
		player_entity_ptr->render_handle = load_render_object(); // DEBUG - this returns 3 instead of 2 for some reason.
		upload_model(player_entity_ptr->render_handle, get_premade_model(0), "entity/pearl");
		render_object_texture_states[player_entity_ptr->render_handle].current_animation_cycle = 3;
	}
}

void tick_game(void) {

	typedef struct four_direction_input_state_t {
		uint8_t flags : 4;
	} four_direction_input_state_t;

	four_direction_input_state_t four_direction_input_state = { .flags = 0 };
	four_direction_input_state.flags |= 1 * is_input_pressed_or_held(GLFW_KEY_W);	// UP
	four_direction_input_state.flags |= 2 * is_input_pressed_or_held(GLFW_KEY_A);	// LEFT
	four_direction_input_state.flags |= 4 * is_input_pressed_or_held(GLFW_KEY_S);	// DOWN
	four_direction_input_state.flags |= 8 * is_input_pressed_or_held(GLFW_KEY_D);	// RIGHT

	if (game_state_bitfield & (uint32_t)GAME_STATE_SCROLLING) {
		if (!is_camera_scrolling()) {
			game_state_bitfield &= ~((uint32_t)GAME_STATE_SCROLLING);
		}
		else {
			settle_render_positions();
			return;
		}
	}

	entity_t *player_entity_ptr = NULL;
	int result = get_entity_ptr(player_entity_handle, &player_entity_ptr);
	if (player_entity_ptr == NULL || result != 0) {
		return;
	}

	static const double max_speed = 0.24;	// Tiles / s
	static const double acceleration_constant = 1.8; // Tiles / s^2
	const double speed = (get_time_ms() - player_entity_ptr->transform.last_stationary_time) * 0.001 * acceleration_constant;

	player_entity_ptr->transform.velocity.r = fmin(speed, max_speed);
	player_entity_ptr->transform.velocity.theta = pi / 2.0;

	static uint32_t animation_cycle = 4;

	switch (four_direction_input_state.flags) {
		case 1:	// 0001 - up only
			player_entity_ptr->transform.velocity.phi = pi / 2.0;
			animation_cycle = 1;
			break;
		case 2:	// 0010 - left only
			player_entity_ptr->transform.velocity.phi = pi;
			animation_cycle = 7;
			break;
		case 4:	// 0100 - down only
			player_entity_ptr->transform.velocity.phi = (3.0 * pi) / 2.0;
			animation_cycle = 5;
			break;
		case 8:	// 1000 - right only
			player_entity_ptr->transform.velocity.phi = 0.0;
			animation_cycle = 3;
			break;
		case 3:	// 0011 - up-left
			player_entity_ptr->transform.velocity.phi = (3.0 * pi) / 4.0;
			animation_cycle = 7;
			break;
		case 6:	// 0110 - down-left
			player_entity_ptr->transform.velocity.phi = (5.0 * pi) / 4.0;
			animation_cycle = 7;
			break;
		case 12:// 1100 - down-right
			player_entity_ptr->transform.velocity.phi = (7.0 * pi) / 4.0;
			animation_cycle = 3;
			break;
		case 9:	// 1001 - up-right
			player_entity_ptr->transform.velocity.phi = pi / 4.0;
			animation_cycle = 3;
			break;
		default:
			player_entity_ptr->transform.velocity.r = 0.0;
			player_entity_ptr->transform.last_stationary_time = get_time_ms();
			animation_cycle -= animation_cycle % 2;
			break;
	}

	tick_entities();
	if (animation_cycle != render_object_texture_states[player_entity_ptr->render_handle].current_animation_cycle) {
		texture_state_set_animation_cycle(&render_object_texture_states[player_entity_ptr->render_handle], animation_cycle);
	}

	const direction_t travel_direction = test_room_travel(player_entity_ptr->transform.position, current_area, current_room_index);
	if ((int)travel_direction > 0) {

		const room_t current_room = current_area.rooms[current_room_index];
		const offset_t room_offset = direction_offset(travel_direction);
		const offset_t next_room_position = offset_add(current_room.position, room_offset);

		const room_t *next_room_ptr = NULL;
		const bool result = area_get_room_ptr(current_area, next_room_position, &next_room_ptr);

		if (result && next_room_ptr != NULL) {
			current_room_index = area_get_room_index(current_area, next_room_position);
			game_state_bitfield |= (uint32_t)GAME_STATE_SCROLLING;
			upload_room_model(*next_room_ptr);
			begin_room_scroll(current_area.room_extent, current_room.position, next_room_ptr->position);
		}
	}
}
