#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#include "config.h"

#include "client.h"
#include "debug.h"
#include "audio/audio_mixer.h"
#include "audio/portaudio/portaudio_manager.h"
#include "game/game.h"
#include "game/entity/entity_manager.h"
#include "game/entity/entity_registry.h"
#include "glfw/glfw_manager.h"
#include "log/Logger.h"
#include "render/renderer.h"

int main(void) {

	logMsg(loggerSystem, LOG_LEVEL_INFO, "Running Pink Pearl version %u.%u", PinkPearl_VERSION_MAJOR, PinkPearl_VERSION_MINOR);
	if (debug_enabled) {
		logMsg(loggerSystem, LOG_LEVEL_WARNING, "Debug mode enabled.");
	}

	init_GLFW();
	init_renderer();
	init_audio_mixer();
	init_portaudio();
	init_entity_registry();
	init_entity_manager();
	start_game();
	//error_queue_flush();

	logMsg(loggerSystem, LOG_LEVEL_INFO, "Ready to play Pink Pearl!");
	run_client();

	terminate_entity_registry();
	terminate_portaudio();
	terminate_audio_mixer();
	terminate_renderer();
	terminate_GLFW();

	logMsg(loggerSystem, LOG_LEVEL_INFO, "Stopping Pink Pearl. Goodbye!");
	//terminate_error_queue();
	
	return 0;
}
