#include "client.h"

#include <stdbool.h>

#include "game/game.h"
#include "glfw/glfw_manager.h"
#include "render/renderer.h"
#include "util/time.h"

static bool client_running = false;

void run_client(void) {

	client_running = true;
	
	static const double ticks_per_second = 20.0;	// ticks / s

	uint64_t time_previous = get_time_ms();	// ms

	float tick_delta_time = 0.0F;

	while (client_running && !should_application_window_close()) {

		const uint64_t time_now = get_time_ms();	// ms
		const uint64_t delta_time = time_now - time_previous;	// ms
		time_previous = time_now;

		// s			  ms	       s / ms    
		tick_delta_time += (float)delta_time * 0.001F;

		while (tick_delta_time >= 1.0F) {
			tick_game();
			tick_delta_time -= 1.0F;
		}

		if (client_running && !should_application_window_close()) {
			render_frame(tick_delta_time);
		}
		else {
			break;
		}
	}
}

void stop_client(void) {
	client_running = false;
}
