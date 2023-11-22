#include "game.h"

#include "render/renderer.h"

#include "area/area.h"
#include "entity/ecs_manager.h"
#include "entity/entity_transform.h"

static area_t current_area = { 0 };

static entity_transform_t player_transform = { 0 };

void start_game(void) {

	init_tECS();

	read_area_data("test", &current_area);
	upload_room_model(current_area.rooms[0]);
}

void tick_game(void) {

}
