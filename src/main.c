#include <unistd.h>
#include "config.h"
#include "client.h"
#include "debug.h"
#include "audio/audio_mixer.h"
#include "audio/portaudio/portaudio_manager.h"
#include "game/game.h"
#include "game/entity/entity_manager.h"
#include "game/entity/EntityRegistry.h"
#include "glfw/GLFWManager.h"
#include "log/Logger.h"
#include "render/RenderManager.h"
#include "util/Random.h"

static const char appVersion[] = "Alpha 0.2";

int main(void) {
	logMsg(loggerSystem, LOG_LEVEL_INFO, "Running Pink Pearl version %s.", appVersion);
	if (debug_enabled) {
		logMsg(loggerSystem, LOG_LEVEL_WARNING, "Debug mode is enabled.");
		logMsg(loggerSystem, LOG_LEVEL_VERBOSE, "Process ID is %i.", getpid());
	}

	initGLFW();
	initRenderManager();
	init_audio_mixer();
	init_portaudio();
	initEntityRegistry();
	init_entity_manager();
	initRandom();
	start_game();

	logMsg(loggerSystem, LOG_LEVEL_INFO, "Ready to play Pink Pearl!");
	runClient();

	endGame();
	terminate_entity_registry();
	terminate_portaudio();
	terminate_audio_mixer();
	terminateRenderManager();
	terminateGLFW();

	logMsg(loggerSystem, LOG_LEVEL_INFO, "Stopping Pink Pearl. Goodbye!");
	return 0;
}
