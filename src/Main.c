#include <unistd.h>
#include "config.h"
#include "debug.h"
#include "audio/audio_mixer.h"
#include "audio/portaudio/portaudio_manager.h"
#include "game/Game.h"
#include "game/entity/entity_manager.h"
#include "game/entity/EntityRegistry.h"
#include "glfw/GLFWManager.h"
#include "log/Logger.h"
#include "render/RenderManager.h"
#include "util/Random.h"
#include "util/Time.h"

static const char appVersion[] = "Alpha 0.2";
static bool appRunning = false;

static void runApp(void);

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
	runApp();

	endGame();
	terminate_entity_registry();
	terminate_portaudio();
	terminate_audio_mixer();
	terminateRenderManager();
	terminateGLFW();

	logMsg(loggerSystem, LOG_LEVEL_INFO, "Stopping Pink Pearl. Goodbye!");
	return 0;
}

static void runApp(void) {
	appRunning = true;
	
	static const double tickPerSecond = 20.0;	// ticks / s
	static const double msPerTick = 1000.0 / tickPerSecond; // ms

	uint64_t previousTime = getMilliseconds();	// ms
	float tickDelta = 0.0F; // Unitless interpolation value

	while (appRunning && !shouldAppWindowClose()) {

		const uint64_t currentTime = getMilliseconds();	// ms
		const uint64_t deltaTime = currentTime - previousTime;	// ms
		previousTime = currentTime;
		tickDelta += (float)(deltaTime / msPerTick);

		while (tickDelta >= 1.0F) {
			tick_game();
			tickDelta -= 1.0F;
		}

		if (appRunning && !shouldAppWindowClose()) {
			const GameState gameState = getGameState();
			renderFrame(tickDelta, areaGetCameraPosition(&currentArea), areaGetProjectionBounds(currentArea), !gameState.paused && !gameState.scrolling);
		} else {
			break;
		}
	}
	
	appRunning = false;
}