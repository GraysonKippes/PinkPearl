#include "game.h"

#include <stdbool.h>
#include <stdint.h>

#include "glfw/input_manager.h"
#include "render/render_object.h"
#include "render/renderer.h"
#include "render/vulkan/vulkan_render.h"

#include "area/area.h"
#include "entity/entity_manager.h"

static area_t current_area = { 0 };

static entity_handle_t player_entity_handle;

void start_game(void) {

	init_entity_manager();
	read_area_data("test", &current_area);
	upload_room_model(current_area.rooms[0]);

	player_entity_handle = load_entity();
	if (!validate_entity_handle(player_entity_handle)) {
		// TODO - error handling
	}

	static const render_handle_t player_render_handle = 2;

	entity_t *player_entity_ptr = NULL;
	int result = get_entity_ptr(player_entity_handle, &player_entity_ptr);
	if (player_entity_ptr != NULL || result == 0) {
		player_entity_ptr->render_handle = player_render_handle;
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

	entity_t *player_entity_ptr = NULL;
	int result = get_entity_ptr(player_entity_handle, &player_entity_ptr);
	if (player_entity_ptr == NULL || result != 0) {
		return;
	}

	player_entity_ptr->transform.velocity.r = 0.24;
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
			animation_cycle -= animation_cycle % 2;
			break;
	}

	tick_entities();

	get_model_texture_ptr(player_entity_ptr->render_handle)->current_animation_cycle = animation_cycle;
}
