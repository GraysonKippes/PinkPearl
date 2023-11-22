#include "game.h"

#include <stdbool.h>
#include <stdint.h>

#include "glfw/input_manager.h"
#include "render/render_object.h"
#include "render/renderer.h"

#include "area/area.h"
#include "entity/ecs_manager.h"
#include "entity/entity_transform.h"

static area_t current_area = { 0 };

static entity_transform_t player_transform = { 0 };

static render_handle_t player_render_handle = 2;

void start_game(void) {

	init_tECS();

	read_area_data("test", &current_area);
	upload_room_model(current_area.rooms[0]);
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
	
	player_transform.velocity.r = 0.24;
	player_transform.velocity.theta = pi / 2.0;

	switch (four_direction_input_state.flags) {
		case 1:	// 0001 - up only
			player_transform.velocity.phi = pi / 2.0;
			break;
		case 2:	// 0010 - left only
			player_transform.velocity.phi = pi;
			break;
		case 4:	// 0100 - down only
			player_transform.velocity.phi = (3.0 * pi) / 2.0;
			break;
		case 8:	// 1000 - right only
			player_transform.velocity.phi = 0.0;
			break;
		case 3:	// 0011 - up-left
			player_transform.velocity.phi = (3.0 * pi) / 4.0;
			break;
		case 6:	// 0110 - down-left
			player_transform.velocity.phi = (5.0 * pi) / 4.0;
			break;
		case 12:// 1100 - down-right
			player_transform.velocity.phi = (7.0 * pi) / 4.0;
			break;
		case 9:	// 1001 - up-right
			player_transform.velocity.phi = pi / 4.0;
			break;
		default:
			player_transform.velocity.r = 0.0;
			break;
	}

	vector3D_rectangular_t old_position = player_transform.position;
	vector3D_rectangular_t position_step = vector3D_polar_to_rectangular(player_transform.velocity);
	player_transform.position = vector3D_rectangular_add(old_position, position_step);

	render_object_positions[player_render_handle].position.x = (float)player_transform.position.x;
	render_object_positions[player_render_handle].position.y = (float)player_transform.position.y;
	render_object_positions[player_render_handle].position.z = (float)player_transform.position.z;
	render_object_positions[player_render_handle].previous_position.x = (float)old_position.x;
	render_object_positions[player_render_handle].previous_position.y = (float)old_position.y;
	render_object_positions[player_render_handle].previous_position.z = (float)old_position.z;
}
