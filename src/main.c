#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "config.h"

#include "client.h"
#include "debug.h"
#include "audio/audio_mixer.h"
#include "audio/portaudio/portaudio_manager.h"
#include "game/game.h"
#include "glfw/glfw_manager.h"
#include "log/logging.h"
#include "render/renderer.h"

int main(void) {

	logf_message(INFO, "Running Pink Pearl version %u.%u", PinkPearl_VERSION_MAJOR, PinkPearl_VERSION_MINOR);
	logf_message(INFO, "C standard: %lu", __STDC_VERSION__);
	if (debug_enabled) {
		log_message(WARNING, "Debug mode enabled.");
	}
	logf_message(VERBOSE, "Resource path: \"%s\"", RESOURCE_PATH);

	init_GLFW();
	init_renderer();
	init_audio_mixer();
	init_portaudio();
	start_game();

	log_message(INFO, "Ready to play Pink Pearl!");
	run_client();

	terminate_portaudio();
	terminate_audio_mixer();
	terminate_renderer();
	terminate_GLFW();

	log_message(INFO, "Stopping Pink Pearl. Goodbye!");
	return 0;
}
