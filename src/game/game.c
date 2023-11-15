#include "game.h"

#include "render/renderer.h"

#include "area/area.h"
#include "entity/ecs_manager.h"

static area_t current_area = { 0 };

void start_game(void) {

	init_tECS();

	read_area_data("test", &current_area);
	upload_room_model(current_area.rooms[0]);
}
